#include "inventory_system.h"
#include "tools.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace Inventory {
    static InventoryItem slots[MAX_SLOTS];
    static int count = 0;
    static int equippedSlot = -1;  // Currently equipped gear slot

    void init() {
        load();
    }

    bool addItem(const std::string& id, const std::string& name, int qty, u32 color) {
        // Check if item already exists
        for (int i = 0; i < count; i++) {
            if (slots[i].itemId == id) {
                slots[i].quantity += qty;
                save();
                return true;
            }
        }
        // Find empty slot
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (slots[i].itemId.empty()) {
                slots[i].itemId = id;
                slots[i].name = name;
                slots[i].quantity = qty;
                slots[i].color = color;
                count++;
                save();
                return true;
            }
        }
        return false; // Full
    }

    bool removeItem(const std::string& id, int qty) {
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (slots[i].itemId == id) {
                slots[i].quantity -= qty;
                if (slots[i].quantity <= 0) {
                    // Remove and shift
                    slots[i] = slots[MAX_SLOTS - 1];
                    slots[MAX_SLOTS - 1] = InventoryItem();
                    count--;
                }
                save();
                return true;
            }
        }
        return false;
    }

    InventoryItem* getItemAtSlot(int slot) {
        if (slot < 0 || slot >= MAX_SLOTS) return nullptr;
        if (slots[slot].itemId.empty()) return nullptr;
        return &slots[slot];
    }

    InventoryItem* findItem(const std::string& id) {
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (slots[i].itemId == id) return &slots[i];
        }
        return nullptr;
    }

    bool equipGear(int slotIndex) {
        if (slotIndex < 0 || slotIndex >= MAX_SLOTS) return false;
        if (slots[slotIndex].itemId.empty()) return false;

        // If it's a HopperBin, toggle its active state
        if (slots[slotIndex].isHopperBin) {
            // Deactivate all other HopperBins first (only one active at a time)
            for (int i = 0; i < MAX_SLOTS; i++) {
                if (slots[i].isHopperBin && i != slotIndex) {
                    slots[i].isActive = false;
                }
            }
            slots[slotIndex].isActive = !slots[slotIndex].isActive;
            equippedSlot = slotIndex;
            applyHopperBinEffects();
            return true;
        }

        // For regular Tools, unequip current gear and equip the new one
        if (equippedSlot >= 0 && equippedSlot < MAX_SLOTS) {
            slots[equippedSlot].isEquipped = false;
        }
        slots[slotIndex].isEquipped = true;
        equippedSlot = slotIndex;
        return true;
    }

    void autoEquipHopperBins() {
        int bestHopperBin = -1;
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (slots[i].isHopperBin && !slots[i].isActive) {
                // Auto-activate the first available HopperBin
                bestHopperBin = i;
                break;
            }
        }
        if (bestHopperBin >= 0) {
            for (int i = 0; i < MAX_SLOTS; i++) {
                if (slots[i].isHopperBin && i != bestHopperBin) {
                    slots[i].isActive = false;
                }
            }
            slots[bestHopperBin].isActive = true;
            equippedSlot = bestHopperBin;
            applyHopperBinEffects();
        }
    }

    void activateTool() {
        // Find the currently equipped Tool (L button = Left Click)
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (slots[i].isTool && slots[i].isEquipped && !slots[i].isHopperBin) {
                // Trigger Tool.Activated event in Lua engine
                printf("[Gear] Activated Tool: %s (Left Click)\n", slots[i].name.c_str());
                break;
            }
        }
    }

    void activateToolSecondary() {
        // Find the currently equipped Tool (R button = Right Click)
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (slots[i].isTool && slots[i].isEquipped && !slots[i].isHopperBin) {
                // Trigger Tool.SecondaryActivated event in Lua engine
                printf("[Gear] Secondary Activated Tool: %s (Right Click)\n", slots[i].name.c_str());
                break;
            }
        }
    }

    void dropEquippedGear() {
        if (equippedSlot < 0 || equippedSlot >= MAX_SLOTS) return;
        InventoryItem& item = slots[equippedSlot];
        if (!item.canBeDropped) return;

        printf("[Gear] Dropped: %s\n", item.name.c_str());
        // Remove from inventory
        removeItem(item.itemId, 1);
        equippedSlot = -1;
        // In a full implementation, this would spawn the gear in Workspace
    }

    void applyHopperBinEffects() {
        // Apply the active HopperBin's effects to player stats
        float speedMod = 0, jumpMod = 0;
        bool flight = false;
        for (int i = 0; i < MAX_SLOTS; i++) {
            if (slots[i].isHopperBin && slots[i].isActive) {
                speedMod = slots[i].walkSpeedModifier;
                jumpMod = slots[i].jumpPowerModifier;
                flight = slots[i].enablesFlight;
                break; // Only one HopperBin active at a time
            }
        }
        // Apply to AnimationSystem
        if (speedMod != 0) {
            AnimationSystem::setWalkSpeed(16.0f + speedMod);
        }
        if (jumpMod != 0) {
            AnimationSystem::setJumpPower(30.0f + jumpMod);
        }
        printf("[HopperBin] Speed: +%.1f, Jump: +%.1f, Flight: %s\n",
               speedMod, jumpMod, flight ? "Yes" : "No");
    }

    int getItemCount() {
        int total = 0;
        for (int i = 0; i < MAX_SLOTS; i++) total += slots[i].quantity;
        return total;
    }

    int getOccupiedSlots() { return count; }
    bool isFull() { return count >= MAX_SLOTS; }

    void clear() {
        for (int i = 0; i < MAX_SLOTS; i++) {
            slots[i] = InventoryItem();
        }
        count = 0;
        save();
    }

    bool save() {
        FILE* f = fopen("sdmc:/OpenBLOX/inventory.dat", "wb");
        if (!f) return false;
        fwrite(&count, sizeof(int), 1, f);
        for (int i = 0; i < MAX_SLOTS; i++) {
            char id[64], name[64], tex[64], tip[128];
            strncpy(id, slots[i].itemId.c_str(), 63); id[63] = 0;
            strncpy(name, slots[i].name.c_str(), 63); name[63] = 0;
            strncpy(tex, slots[i].textureId.c_str(), 63); tex[63] = 0;
            strncpy(tip, slots[i].toolTip.c_str(), 127); tip[127] = 0;
            fwrite(id, 64, 1, f);
            fwrite(name, 64, 1, f);
            fwrite(&slots[i].quantity, sizeof(int), 1, f);
            fwrite(&slots[i].color, sizeof(u32), 1, f);
            fwrite(tex, 64, 1, f);
            fwrite(tip, 128, 1, f);
            fwrite(&slots[i].isGear, sizeof(bool), 1, f);
            fwrite(&slots[i].isHopperBin, sizeof(bool), 1, f);
            fwrite(&slots[i].isTool, sizeof(bool), 1, f);
            fwrite(&slots[i].canBeDropped, sizeof(bool), 1, f);
            fwrite(&slots[i].isActive, sizeof(bool), 1, f);
            fwrite(&slots[i].isEquipped, sizeof(bool), 1, f);
            fwrite(&slots[i].walkSpeedModifier, sizeof(float), 1, f);
            fwrite(&slots[i].jumpPowerModifier, sizeof(float), 1, f);
            fwrite(&slots[i].enablesFlight, sizeof(bool), 1, f);
            fwrite(&slots[i].enablesBuilding, sizeof(bool), 1, f);
        }
        fwrite(&equippedSlot, sizeof(int), 1, f);
        fclose(f);
        return true;
    }

    bool load() {
        FILE* f = fopen("sdmc:/OpenBLOX/inventory.dat", "rb");
        if (!f) return true;
        fread(&count, sizeof(int), 1, f);
        count = std::min(count, MAX_SLOTS);
        for (int i = 0; i < MAX_SLOTS; i++) {
            char id[64], name[64], tex[64], tip[128];
            fread(id, 64, 1, f);
            fread(name, 64, 1, f);
            fread(&slots[i].quantity, sizeof(int), 1, f);
            fread(&slots[i].color, sizeof(u32), 1, f);
            fread(tex, 64, 1, f);
            fread(tip, 128, 1, f);
            fread(&slots[i].isGear, sizeof(bool), 1, f);
            fread(&slots[i].isHopperBin, sizeof(bool), 1, f);
            fread(&slots[i].isTool, sizeof(bool), 1, f);
            fread(&slots[i].canBeDropped, sizeof(bool), 1, f);
            fread(&slots[i].isActive, sizeof(bool), 1, f);
            fread(&slots[i].isEquipped, sizeof(bool), 1, f);
            fread(&slots[i].walkSpeedModifier, sizeof(float), 1, f);
            fread(&slots[i].jumpPowerModifier, sizeof(float), 1, f);
            fread(&slots[i].enablesFlight, sizeof(bool), 1, f);
            fread(&slots[i].enablesBuilding, sizeof(bool), 1, f);
            slots[i].itemId = id;
            slots[i].name = name;
            slots[i].textureId = tex;
            slots[i].toolTip = tip;
        }
        fread(&equippedSlot, sizeof(int), 1, f);
        fclose(f);
        return true;
    }

    void shutdown() {
        save();
    }
}