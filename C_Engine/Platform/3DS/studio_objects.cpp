#include "studio_objects.h"
#include <algorithm>
#include <string.h>

namespace StudioObjects {

static std::vector<ObjectTemplate> g_templates;

void init() {
    g_templates.clear();

    // =========================================
    // MODEL OBJECTS (BasePart, MeshPart, etc.)
    // =========================================
    ObjectTemplate part;
    part.className = "BasePart"; part.displayName = "Part"; part.icon = 'P';
    part.category = ObjectCategory::Model;
    part.defaultPosition[0] = 0; part.defaultPosition[1] = 5; part.defaultPosition[2] = 0;
    part.defaultSize[0] = 4; part.defaultSize[1] = 4; part.defaultSize[2] = 4;
    part.defaultColor[0] = 1.0f; part.defaultColor[1] = 0.5f; part.defaultColor[2] = 0.5f;
    g_templates.push_back(part);

    ObjectTemplate meshpart;
    meshpart.className = "MeshPart"; meshpart.displayName = "Mesh Part"; meshpart.icon = 'M';
    meshpart.category = ObjectCategory::Model;
    meshpart.defaultColor[0] = 0.8f; meshpart.defaultColor[1] = 0.8f; meshpart.defaultColor[2] = 0.8f;
    g_templates.push_back(meshpart);

    ObjectTemplate model;
    model.className = "Model"; model.displayName = "Model"; model.icon = '#';
    model.category = ObjectCategory::Model;
    g_templates.push_back(model);

    ObjectTemplate folder;
    folder.className = "Folder"; folder.displayName = "Folder"; folder.icon = 'F';
    folder.category = ObjectCategory::Model;
    g_templates.push_back(folder);

    ObjectTemplate attachment;
    attachment.className = "Attachment"; attachment.displayName = "Attachment"; attachment.icon = 'A';
    attachment.category = ObjectCategory::Model;
    g_templates.push_back(attachment);

    ObjectTemplate unionop;
    unionop.className = "UnionOperation"; unionop.displayName = "Union"; unionop.icon = 'U';
    unionop.category = ObjectCategory::Model;
    g_templates.push_back(unionop);

    ObjectTemplate wedge;
    wedge.className = "WedgePart"; wedge.displayName = "Wedge"; wedge.icon = 'W';
    wedge.category = ObjectCategory::Model;
    wedge.defaultSize[0] = 4; wedge.defaultSize[1] = 4; wedge.defaultSize[2] = 8;
    g_templates.push_back(wedge);

    ObjectTemplate cylinder;
    cylinder.className = "CylinderPart"; cylinder.displayName = "Cylinder"; cylinder.icon = 'O';
    cylinder.category = ObjectCategory::Model;
    cylinder.defaultSize[0] = 4; cylinder.defaultSize[1] = 4; cylinder.defaultSize[2] = 4;
    g_templates.push_back(cylinder);

    ObjectTemplate cornerwedge;
    cornerwedge.className = "CornerWedgePart"; cornerwedge.displayName = "Corner Wedge"; cornerwedge.icon = 'V';
    cornerwedge.category = ObjectCategory::Model;
    g_templates.push_back(cornerwedge);

    // =========================================
    // UI OBJECTS (ScreenGui, Frames, etc.)
    // =========================================
    ObjectTemplate screengui;
    screengui.className = "ScreenGui"; screengui.displayName = "ScreenGui"; screengui.icon = 'S';
    screengui.category = ObjectCategory::UI;
    g_templates.push_back(screengui);

    ObjectTemplate surfacegui;
    surfacegui.className = "SurfaceGui"; surfacegui.displayName = "SurfaceGui"; surfacegui.icon = 's';
    surfacegui.category = ObjectCategory::UI;
    g_templates.push_back(surfacegui);

    ObjectTemplate frame;
    frame.className = "Frame"; frame.displayName = "Frame"; frame.icon = '[';
    frame.category = ObjectCategory::UI;
    g_templates.push_back(frame);

    ObjectTemplate textlabel;
    textlabel.className = "TextLabel"; textlabel.displayName = "TextLabel"; textlabel.icon = 'T';
    textlabel.category = ObjectCategory::UI;
    g_templates.push_back(textlabel);

    ObjectTemplate textbutton;
    textbutton.className = "TextButton"; textbutton.displayName = "TextButton"; textbutton.icon = 'B';
    textbutton.category = ObjectCategory::UI;
    g_templates.push_back(textbutton);

    ObjectTemplate imagebutton;
    imagebutton.className = "ImageButton"; imagebutton.displayName = "ImageButton"; imagebutton.icon = 'I';
    imagebutton.category = ObjectCategory::UI;
    g_templates.push_back(imagebutton);

    ObjectTemplate textbox;
    textbox.className = "TextBox"; textbox.displayName = "TextBox"; textbox.icon = '_';
    textbox.category = ObjectCategory::UI;
    g_templates.push_back(textbox);

    ObjectTemplate scrollingframe;
    scrollingframe.className = "ScrollingFrame"; scrollingframe.displayName = "ScrollingFrame"; scrollingframe.icon = '|';
    scrollingframe.category = ObjectCategory::UI;
    g_templates.push_back(scrollingframe);

    ObjectTemplate textchat;
    textchat.className = "TextChat"; textchat.displayName = "TextChat"; textchat.icon = 'c';
    textchat.category = ObjectCategory::UI;
    g_templates.push_back(textchat);

    ObjectTemplate uicorner;
    uicorner.className = "UICorner"; uicorner.displayName = "UICorner"; uicorner.icon = '(';
    uicorner.category = ObjectCategory::UI;
    g_templates.push_back(uicorner);

    ObjectTemplate uipadding;
    uipadding.className = "UIPadding"; uipadding.displayName = "UIPadding"; uipadding.icon = '+';
    uipadding.category = ObjectCategory::UI;
    g_templates.push_back(uipadding);

    ObjectTemplate uistroke;
    uistroke.className = "UIStroke"; uistroke.displayName = "UIStroke"; uistroke.icon = '=';
    uistroke.category = ObjectCategory::UI;
    g_templates.push_back(uistroke);

    ObjectTemplate uilistlayout;
    uilistlayout.className = "UIListLayout"; uilistlayout.displayName = "UIListLayout"; uilistlayout.icon = 'L';
    uilistlayout.category = ObjectCategory::UI;
    g_templates.push_back(uilistlayout);

    ObjectTemplate uigridlayout;
    uigridlayout.className = "UIGridLayout"; uigridlayout.displayName = "UIGridLayout"; uigridlayout.icon = 'G';
    uigridlayout.category = ObjectCategory::UI;
    g_templates.push_back(uigridlayout);

    ObjectTemplate uipagelayout;
    uipagelayout.className = "UIPageLayout"; uipagelayout.displayName = "UIPageLayout"; uipagelayout.icon = 'p';
    uipagelayout.category = ObjectCategory::UI;
    g_templates.push_back(uipagelayout);

    ObjectTemplate uiflexlayout;
    uiflexlayout.className = "UIFlexLayout"; uiflexlayout.displayName = "UIFlexLayout"; uiflexlayout.icon = 'f';
    uiflexlayout.category = ObjectCategory::UI;
    g_templates.push_back(uiflexlayout);

    // =========================================
    // VALUES (StringValue, IntValue, etc.)
    // =========================================
    ObjectTemplate stringvalue;
    stringvalue.className = "StringValue"; stringvalue.displayName = "StringValue"; stringvalue.icon = '"';
    stringvalue.category = ObjectCategory::Values;
    g_templates.push_back(stringvalue);

    ObjectTemplate intval;
    intval.className = "IntValue"; intval.displayName = "IntValue"; intval.icon = 'i';
    intval.category = ObjectCategory::Values;
    g_templates.push_back(intval);

    ObjectTemplate boolval;
    boolval.className = "BoolValue"; boolval.displayName = "BoolValue"; boolval.icon = '!';
    boolval.category = ObjectCategory::Values;
    g_templates.push_back(boolval);

    ObjectTemplate numberval;
    numberval.className = "NumberValue"; numberval.displayName = "NumberValue"; numberval.icon = 'n';
    numberval.category = ObjectCategory::Values;
    g_templates.push_back(numberval);

    ObjectTemplate objval;
    objval.className = "ObjectValue"; objval.displayName = "ObjectValue"; objval.icon = 'o';
    objval.category = ObjectCategory::Values;
    g_templates.push_back(objval);

    ObjectTemplate binarystrval;
    binarystrval.className = "BinaryStringValue"; binarystrval.displayName = "BinaryStringValue"; binarystrval.icon = '%';
    binarystrval.category = ObjectCategory::Values;
    g_templates.push_back(binarystrval);

    ObjectTemplate color3val;
    color3val.className = "Color3Value"; color3val.displayName = "Color3Value"; color3val.icon = 'C';
    color3val.category = ObjectCategory::Values;
    g_templates.push_back(color3val);

    ObjectTemplate brickcolorval;
    brickcolorval.className = "BrickColorValue"; brickcolorval.displayName = "BrickColorValue"; brickcolorval.icon = 'R';
    brickcolorval.category = ObjectCategory::Values;
    g_templates.push_back(brickcolorval);

    ObjectTemplate contentval;
    contentval.className = "Content"; contentval.displayName = "Content"; contentval.icon = '>';
    contentval.category = ObjectCategory::Values;
    g_templates.push_back(contentval);

    // =========================================
    // CONSTRAINTS (Weld, Motor6D, etc.)
    // =========================================
    ObjectTemplate weld;
    weld.className = "Weld"; weld.displayName = "Weld"; weld.icon = '@';
    weld.category = ObjectCategory::Constraints;
    g_templates.push_back(weld);

    ObjectTemplate motor;
    motor.className = "Motor6D"; motor.displayName = "Motor6D"; motor.icon = '@';
    motor.category = ObjectCategory::Constraints;
    g_templates.push_back(motor);

    ObjectTemplate hinge;
    hinge.className = "HingeConstraint"; hinge.displayName = "HingeConstraint"; hinge.icon = 'h';
    hinge.category = ObjectCategory::Constraints;
    g_templates.push_back(hinge);

    ObjectTemplate ballsocket;
    ballsocket.className = "BallSocketConstraint"; ballsocket.displayName = "BallSocket"; ballsocket.icon = 'b';
    ballsocket.category = ObjectCategory::Constraints;
    g_templates.push_back(ballsocket);

    ObjectTemplate rocket;
    rocket.className = "RocketPropulsion"; rocket.displayName = "RocketPropulsion"; rocket.icon = 'r';
    rocket.category = ObjectCategory::Constraints;
    g_templates.push_back(rocket);

    ObjectTemplate alignpos;
    alignpos.className = "AlignPosition"; alignpos.displayName = "AlignPosition"; alignpos.icon = 'a';
    alignpos.category = ObjectCategory::Constraints;
    g_templates.push_back(alignpos);

    ObjectTemplate alignorient;
    alignorient.className = "AlignOrientation"; alignorient.displayName = "AlignOrientation"; alignorient.icon = 'a';
    alignorient.category = ObjectCategory::Constraints;
    g_templates.push_back(alignorient);

    ObjectTemplate spring;
    spring.className = "SpringConstraint"; spring.displayName = "SpringConstraint"; spring.icon = '~';
    spring.category = ObjectCategory::Constraints;
    g_templates.push_back(spring);

    ObjectTemplate beam;
    beam.className = "Beam"; beam.displayName = "Beam"; beam.icon = '-';
    beam.category = ObjectCategory::Constraints;
    g_templates.push_back(beam);

    // Sort templates by category and name
    std::sort(g_templates.begin(), g_templates.end(), [](const ObjectTemplate& a, const ObjectTemplate& b) {
        if (a.category != b.category) return a.category < b.category;
        return a.className < b.className;
    });
}

const ObjectTemplate* getTemplate(const std::string& className) {
    for (const auto& tmpl : g_templates) {
        if (tmpl.className == className) return &tmpl;
    }
    return nullptr;
}

const std::vector<ObjectTemplate>& getAllTemplates() {
    return g_templates;
}

std::vector<const ObjectTemplate*> getTemplatesByCategory(ObjectCategory category) {
    std::vector<const ObjectTemplate*> result;
    for (const auto& tmpl : g_templates) {
        if (tmpl.category == category) result.push_back(&tmpl);
    }
    return result;
}

void shutdown() {
    g_templates.clear();
}
}