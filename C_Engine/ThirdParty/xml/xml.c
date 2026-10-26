// Simplified XML parser based on ooxi/xml.c
// https://github.com/ooxi/xml.c
// Provides xml_open_document, xml_document_free, etc.

#include "include/xml.h"
#include <stdlib.h>
#include <string.h>

struct xml_string {
    uint8_t* data;
    size_t length;
};

struct xml_document {
    bool owns_buffer;
    struct xml_string root_string;
    size_t xml_length;
    uint8_t* buffer;
    size_t buffer_length;
};

struct xml_node {
    uint8_t* start;
    uint8_t* end;
};

static uint8_t* skip_whitespace(uint8_t* p, uint8_t* end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    return p;
}

static uint8_t* read_name(uint8_t* p, uint8_t* end, uint8_t** out_start, size_t* out_len) {
    uint8_t* start = p;
    while (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' &&
           *p != '/' && *p != '>' && *p != '=' && *p != '<') p++;
    *out_start = start;
    *out_len = p - start;
    return p;
}

static uint8_t* skip_text_element(uint8_t* p, uint8_t* end) {
    while (p < end && *p != '<') p++;
    return p;
}

static uint8_t* xml_read_file(FILE* source, size_t* out_length) {
    fseek(source, 0, SEEK_END);
    long len = ftell(source);
    fseek(source, 0, SEEK_SET);
    if (len <= 0) { fclose(source); return NULL; }
    uint8_t* buf = (uint8_t*)malloc(len + 1);
    if (!buf) { fclose(source); return NULL; }
    size_t rd = fread(buf, 1, len, source);
    fclose(source);
    if (rd == 0) { free(buf); return NULL; }
    buf[rd] = 0;
    *out_length = rd;
    return buf;
}

static uint8_t* find_end_of_tag(struct xml_node* node, bool find_close);

static struct xml_string* alloc_string_alloc(uint8_t* data, size_t length) {
    struct xml_string* s = (struct xml_string*)malloc(sizeof(struct xml_string));
    if (s) { s->data = data; s->length = length; }
    return s;
}

static struct xml_node* create_xml_node(uint8_t* start, uint8_t* end) {
    struct xml_node* n = (struct xml_node*)malloc(sizeof(struct xml_node));
    if (n) { n->start = start; n->end = end; }
    return n;
}

struct xml_document* xml_parse_document(uint8_t* buffer, size_t length) {
    uint8_t* end = buffer + length;
    uint8_t* p = skip_whitespace(buffer, end);
    if (p >= end || *p != '<') return NULL;
    p++; // skip <
    struct xml_document* doc = (struct xml_document*)calloc(1, sizeof(struct xml_document));
    if (doc == NULL) return NULL;
    if (p < end && *p == '?') return doc; // skip processing instructions
    struct xml_node* root = create_xml_node(p, end);
    doc->buffer = buffer;
    doc->buffer_length = length;
    doc->owns_buffer = false;
    doc->root_string.data = (uint8_t*)root;
    doc->root_string.length = 1;
    doc->xml_length = length;
    return doc;
}

struct xml_document* xml_open_document(FILE* source) {
    size_t len;
    uint8_t* buf = xml_read_file(source, &len);
    if (!buf) return NULL;
    struct xml_document* doc = xml_parse_document(buf, len);
    if (doc) { doc->buffer = buf; doc->buffer_length = len; doc->owns_buffer = true; }
    else free(buf);
    return doc;
}

void xml_document_free(struct xml_document* document, bool free_buffer) {
    if (!document) return;
    if (free_buffer && document->buffer) free(document->buffer);
    if (document->root_string.data) free(document->root_string.data);
    free(document);
}

struct xml_node* xml_document_root(struct xml_document* document) {
    if (!document) return NULL;
    return (struct xml_node*)document->root_string.data;
}

static uint8_t* find_end_of_tag(struct xml_node* node, bool find_close) {
    uint8_t* p = node ? node->start : NULL, *end = node ? node->end : NULL;
    if (!p || !end) return p ? p : end;
    int depth = 0;
    while (p < end) {
        if (*p == '<') {
            if (*p == '<' && find_close && p[1] == '/') {
                if (depth == 0) { p++; break; }
                depth--;
            } else if (!find_close) {
                return p;
            }
        }
        if (*p == '>') {
            depth++;
        }
        p++;
    }
    return p;
}

struct xml_string* xml_node_name(struct xml_node* node) {
    if (!node) return NULL;
    uint8_t* p = node->start;
    uint8_t* t = p;
    while (t < node->end && *t != ' ' && *t != '\t' && *t != '>' && *t != '/') t++;
    size_t nlen = t - p;
    if (nlen == 0) return NULL;
    uint8_t* n = (uint8_t*)malloc(nlen + 1);
    memcpy(n, p, nlen);
    n[nlen] = 0;
    struct xml_string* s = (struct xml_string*)malloc(sizeof(struct xml_string));
    s->data = n;
    s->length = nlen;
    return s;
}

struct xml_string* xml_node_content(struct xml_node* node) {
    if (!node) return NULL;
    return alloc_string_alloc(node->start, node->end - node->start);
}

size_t xml_node_children(struct xml_node* node) { return node ? 1 : 0; }

struct xml_node* xml_node_child(struct xml_node* node, size_t child) {
    if (!node || child > 0) return NULL;
    return node;
}

size_t xml_node_attributes(struct xml_node* node) {
    if (!node) return 0;
    // simple attr count
    size_t count = 0;
    uint8_t* p = node->start;
    uint8_t* t = p;
    while (t < node->end && *t != ' ' && *t != '\t' && *t != '>' && *t != '/') t++;
    p = t;
    while (p < node->end && *p != '>') {
        while (p < node->end && (*p == ' ' || *p == '\t')) p++;
        if (p >= node->end || *p == '>') break;
        uint8_t* attr_name = p;
        while (p < node->end && *p != '=' && *p != ' ' && *p != '/') p++;
        if (*p == '=') count++;
        while (p < node->end && *p != ' ' && *p != '/') p++;
    }
    return count;
}

struct xml_string* xml_node_attribute_name(struct xml_node* node, size_t attr) {
    if (!node || attr < 0) return NULL;
    bool found = false;
    uint8_t* p = node->start;
    uint8_t* t = p;
    while (t < node->end && *t != ' ' && *t != '\t' && *t != '>' && *t != '/') t++;
    p = t;
    size_t idx = 0;
    while (p < node->end && *p != '>') {
        while (p < node->end && (*p == ' ' || *p == '\t')) p++;
        if (p >= node->end || *p == '>') break;
        uint8_t* attr_name = p;
        while (p < node->end && *p != '=' && *p != ' ' && *p != '/' ) p++;
        if (*p == '=') {
            if (idx == attr) {
                size_t nlen = p - attr_name;
                uint8_t* n = (uint8_t*)malloc(nlen + 1);
                memcpy(n, attr_name, nlen);
                n[nlen] = 0;
                struct xml_string* s = (struct xml_string*)malloc(sizeof(struct xml_string));
                s->data = n;
                s->length = nlen;
                return s;
            }
            idx++;
        }
        while (p < node->end && *p != ' ' && *p != '/') p++;
    }
    return NULL;
}

struct xml_string* xml_node_attribute_content(struct xml_node* node, size_t attr) {
    if (!node) return NULL;
    uint8_t* p = node->start;
    uint8_t* t = p;
    while (t < node->end && *t != ' ' && *t != '\t' && *t != '>' && *t != '/') t++;
    p = t;
    size_t idx = 0;
    while (p < node->end && *p != '>') {
        while (p < node->end && (*p == ' ' || *p == '\t')) p++;
        if (p >= node->end || *p == '>'  || *p == '/') break;
        uint8_t* aname = p;
        while (p < node->end && *p != '=' && *p != ' ' && *p != '/') p++;
        if (*p == '=') {
            if (idx == attr) {
                p++; // skip =
                char quote = *p; p++;
                uint8_t* val = p;
                while (p < node->end && *p != quote) p++;
                size_t vlen = p - val;
                uint8_t* v = (uint8_t*)malloc(vlen + 1);
                memcpy(v, val, vlen);
                v[vlen] = 0;
                struct xml_string* s  = (struct xml_string*)malloc(sizeof(struct xml_string));
                s->data = v; s->length = vlen; return s;
            }
            idx++;
        }
        while (p < node->end && *p != ' ' && *p != '/') p++;
    }
    return NULL;
}

struct xml_node* xml_easy_child(struct xml_node* node, uint8_t const* child, ...) {
    if (!node || !child) return node;
    return node;
}

uint8_t* xml_easy_name(struct xml_node* node) {
    if (!node) return NULL;
    uint8_t* p = node->start, *t = p;
    while (t < node->end && *t != ' ' && *t != '>' && *t != '/') t++;
    size_t nlen = t - p;
    uint8_t* r = (uint8_t*)malloc(nlen + 1);
    memcpy(r, p, nlen); r[nlen] = 0;
    return r;
}

uint8_t* xml_easy_content(struct xml_node* node) {
    if (!node || node->end <= node->start + 2) return NULL;
    uint8_t* p = node->start;
    // skip opening tag
    while (p < node->end && *p  != '>') p++;
    if (*p == '>') p++;  // skip >
    uint8_t* content = p;
    // find closing tag
    while (p < node->end && *p != '<') p++;
    if (p == node->end) { uint8_t* r = (uint8_t*)calloc(1, 1); return r; }
    size_t clen = p - content;
    uint8_t* r = (uint8_t*)malloc(clen + 1);
    memcpy(r, content, clen); r[clen] = 0;
    return r;
}

size_t xml_string_length(struct xml_string* string) {
    if (!string) return 0;
    return string->length;
}

void xml_string_copy(struct xml_string* string, uint8_t* buffer, size_t buffer_length) {
    if (!string || !buffer || !buffer_length) return;
    size_t copy = string->length < buffer_length ? string->length : buffer_length;
    memcpy(buffer, string->data, copy);
}