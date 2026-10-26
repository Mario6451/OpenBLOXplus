#pragma once
#include <string>
#include <vector>
#include <3ds.h>

namespace Inventory {
    struct InventoryItem {
        std::string itemId;
        std::string name;
        int quantity;
        u32 color;

        // Gear-specific fields
        bool isGear = false;           // Is this a Gear (Tool or HopperBin)?
        bool isHopperBin = false;      // Is this a HopperBin (auto-equips, continuous effect)?
        bool isTool = false;           // Is this a Tool (requires L/R click to activate)?
        std::string textureId;         // Gear icon texture
        std::string toolTip;           // Gear description
        bool canBeDropped = true;      // Can the gear be dropped into workspace?
        bool isActive = false;         // For HopperBins: is the effect currently active?
        bool isEquipped = false;       // Is this gear currently equipped in hand?

        // HopperBin effects
        float walkSpeedModifier = 0;   // Extra walk speed
        float jumpPowerModifier = 0;   // Extra jump power
        bool enablesFlight = false;    // Grants flight
        bool enablesBuilding = false;  // Allows building in games with BuildPermission=None
    };

    const int MAX_SLOTS = 20;

    void init();

    // Add item to inventory
    bool addItem(const std::string& id, const std::string& name, int qty = 1, u32 color = 0xFFAAAAAA);

    // Remove item from inventory
    bool removeItem(const std::string& id, int qty = 1);

    // Get item at slot
    InventoryItem* getItemAtSlot(int slot);

    // Find item by ID
    InventoryItem* findItem(const std::string& id);

    // Equip gear (Tool or HopperBin)
    bool equipGear(int slotIndex);

    // Auto-equip HopperBins (highest priority active)
    void autoEquipHopperBins();

    // Activate equipped Tool (L button = Left Click)
    void activateTool();

    // Activate equipped Tool secondary (R button = Right Click)
    void activateToolSecondary();

    // Drop equipped gear into workspace
    void dropEquippedGear();

    // Apply HopperBin effects to player stats
    void applyHopperBinEffects();

    // Get total items count
    int getItemCount();

    // Get occupied slot count
    int getOccupiedSlots();

    // Check if inventory is full
    bool isFull();

    // Clear all items
    void clear();

    // Save/load
    bool save();
    bool load();

    void shutdown();
}