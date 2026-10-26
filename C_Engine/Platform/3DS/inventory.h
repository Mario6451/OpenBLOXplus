#pragma once

#include <string>
#include <vector>
#include <map>

// Inventory system - pick up, drop, and store items

namespace Inventory {

    struct Item {
        std::string name;           // Display name
        std::string assetId;        // Asset identifier
        int quantity = 1;           // Stackable quantity
        bool isTool = false;        // Equippable tool
        bool isConsumable = false;  // Usable item
        bool isEquipped = false;    // Currently in hand
    };

    // Initialize inventory system
    void init();

    // Add item to inventory
    bool addItem(const std::string& name, const std::string& assetId, int quantity = 1);

    // Remove item from inventory
    bool removeItem(int index, int quantity = 1);

    // Get item at index
    Item* getItem(int index);

    // Get item by name
    Item* getItemByName(const std::string& name);

    // Equip item (move to hand slot)
    bool equipItem(int index);

    // Unequip current item
    void unequipItem();

    // Get equipped item
    Item* getEquippedItem();

    // Get inventory size
    int getSize();

    // Get max capacity
    int getMaxCapacity();

    // Check if inventory is full
    bool isFull();

    // Clear inventory
    void clear();

    // Save/load inventory
    bool save(const std::string& path);
    bool load(const std::string& path);

    // Get item at slot (same as getItem, for UI compatibility)
    Item getItemAtSlot(int index);

    
    // Equip gear (same as equipItem, returns true on success)
    bool equipGear(int index);

    // Shutdown
    void shutdown();
}