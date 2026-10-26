#include "inventory.h"
#include <algorithm>
#include <stdio.h>

using Item = Inventory::Item;
static std::vector<Item> items;
static const int maxCapacity = 20;

void Inventory::init() {
    items.clear();
}

bool Inventory::addItem(const std::string& name, const std::string& assetId, int quantity) {
    if (getSize() + (quantity > 0 ? 1 : 0) > maxCapacity) return false;

    // Check if stackable
    for (auto& item : items) {
        if (item.assetId == assetId && !item.isTool && quantity > 0) {
            item.quantity += quantity;
            return true;
        }
    }

    Item newItem;
    newItem.name = name;
    newItem.assetId = assetId;
    newItem.quantity = quantity;
    items.push_back(newItem);
    return true;
}

bool Inventory::removeItem(int index, int quantity) {
    if (index < 0 || index >= (int)items.size()) return false;
    
    if (items[index].quantity > quantity) {
        items[index].quantity -= quantity;
    } else {
        items.erase(items.begin() + index);
    }
    return true;
}

Item* Inventory::getItem(int index) {
    if (index < 0 || index >= (int)items.size()) return nullptr;
    return &items[index];
}

Item* Inventory::getItemByName(const std::string& name) {
    for (auto& item : items) {
        if (item.name == name) return &item;
    }
    return nullptr;
}

bool Inventory::equipItem(int index) {
    if (index < 0 || index >= (int)items.size()) return false;
    if (!items[index].isTool) return false;
    
    unequipItem();
    items[index].isEquipped = true;
    return true;
}

void Inventory::unequipItem() {
    for (auto& item : items) {
        item.isEquipped = false;
    }
}

Item* Inventory::getEquippedItem() {
    for (auto& item : items) {
        if (item.isEquipped) return &item;
    }
    return nullptr;
}

int Inventory::getSize() {
    return (int)items.size();
}

int Inventory::getMaxCapacity() {
    return maxCapacity;
}

bool Inventory::isFull() {
    return getSize() >= maxCapacity;
}

void Inventory::clear() {
    items.clear();
}

bool Inventory::save(const std::string& path) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return false;
    
    fprintf(f, "%d\n", (int)items.size());
    for (const auto& item : items) {
        fprintf(f, "%s\n%s\n%d\n%d\n%d\n%d\n",
            item.name.c_str(),
            item.assetId.c_str(),
            item.quantity,
            item.isTool ? 1 : 0,
            item.isConsumable ? 1 : 0,
            item.isEquipped ? 1 : 0);
    }
    fclose(f);
    return true;
}

bool Inventory::load(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return false;
    
    clear();
    int count = 0;
    if (fscanf(f, "%d\n", &count) != 1) { fclose(f); return false; }
    
    for (int i = 0; i < count; i++) {
        char name[256], asset[256];
        int qty, tool, consumable, equipped;
        if (fscanf(f, "%s\n%s\n%d\n%d\n%d\n%d\n",
            name, asset, &qty, &tool, &consumable, &equipped) != 6) break;
        
        Item item;
        item.name = name;
        item.assetId = asset;
        item.quantity = qty;
        item.isTool = (tool != 0);
        item.isConsumable = (consumable != 0);
        item.isEquipped = (equipped != 0);
        items.push_back(item);
    }
    fclose(f);
    return true;
}

Inventory::Item Inventory::getItemAtSlot(int index) {
    Item empty;
    if (index < 0 || index >= (int)items.size()) return empty;
    return items[index];
}

bool Inventory::equipGear(int index) {
    return equipItem(index);
}

void Inventory::shutdown() {
    items.clear();
}