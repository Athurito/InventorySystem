# RPGInventory Plugin Spezifikation (aktualisiert)

## Ziel
Ein Inventory-Plugin für ein RPG in **Unreal Engine 5.6**, basierend auf dem **Entity-Component-Pattern** mit **GAS**.  
Modular, server-autorisiert, performant, **CommonUI**-basiert (Controller-Support), **Drag & Drop komplett in C++**.

---

## Architektur-Überblick

ItemDefinition (PrimaryDataAsset)
├─ ItemFragments (DataAssets, Feature-Module)
│ ├─ Stackable / Consumable / Equippable / Visual / Weight / Tags / ...
│ └─ UI-Fragmente (nur UI: Tooltip/Description/Badges/Stats-Slices)
└─ Runtime: UItemInstance (per Eintrag, hält Instanzdaten)

URPGInventoryComponent (Player/Owner-gebunden, FastArray)
URPGStorageContainerComponent (Weltobjekt-gebunden, FastArray, multi-user)

ARPGWorldPickup (IInteractable)
└─ UItemPickupComponent ← steuert komplettes Verhalten (Spawn/Init/Interact/Rep)


**Kernprinzipien**
- **Fragment-getriebene Logik:** Interaktion, Consume/Equip, UI-Beschreibung – alles wird von den vorhandenen **ItemFragments** bestimmt (kein God-Item).
- **Server-Authority:** Alle Mutationen laufen über den Server (RPC/Validation). FastArray für effiziente Replikation.
- **Ein-Slot-Items:** Jedes Item belegt **genau 1 Slot** (kein 2D-Tetris).
- **CommonUI-First:** Alle Widgets als **C++ CommonUI**-Klassen, Präfix **`CUI_`**; Controller-Navigation inklusive.

---

## Items & Fragmente

### Basis
- **`UItemDefinition`** (`PrimaryDataAsset`, Asset-Type z. B. `ItemDef`)
    - Liste von **`UItemFragment`**-DA-Referenzen.
- **`UItemFragment`** (abstrakt, DA)
    - **Gameplay-Fragmente** (Logik/Gameplay):
        - `UItemFragment_Stackable` (MaxStack)
        - `UItemFragment_Consumable` (GE/GA, CooldownTag)
        - `UItemFragment_Equippable` (AllowedSlots: `FGameplayTagContainer`, GrantedAbilities, Attachment)
        - `UItemFragment_Visual` (Icon, Mesh, PickupTemplate)
        - `UItemFragment_Weight` (Weight)
        - `UItemFragment_Tags` (freie GameplayTags)
    - **UI-Fragmente** (rein für Darstellung/Tooltip/Badges):
        - `UItemFragment_UI_Description` (Title, Flavor, RichText)
        - `UItemFragment_UI_StatsSlice` (Key/Value-Slices, z. B. Damage 12–15)
        - `UItemFragment_UI_Badges` (z. B. „Selten“, „Quest“)
        - -> Zweck: **Composite-Pattern** im Tooltip (siehe UI)
- **`UItemInstance`** (UObject)
    - Referenz auf Definition + Instanzdaten (StackCount, Rolls, Durability, Seeds, Cosmetic).

### Fragment-gesteuerte Interaktion
- Alle Interaktions-Entscheidungen laufen über Fragment-Hooks:
    - `bool CanInteract(…);`
    - `EInteractResult OnInteract(…);`
    - `bool CanConsume(…);`
    - `bool CanEquip(…);` etc.
- **UItemPickupComponent** ruft diese Hooks auf, nicht der Actor selbst.

---

## World Pickup

### `ARPGWorldPickup`
- Implementiert `IInteractable`.
- Hält **`UItemPickupComponent`** (UActorComponent) – **macht „alles“**:
    - Init mit `ItemDefinition` + `StackCount`.
    - Authority-Check; serverseitige Aufnahme -> ruft Inventory-API (Server) auf.
    - Optional Pooling/Wiederverwendung.
- Visuals/Collision vom `UItemFragment_Visual`.

**Warum Component?** Klare Trennung: Actor = Hülle, **ItemPickupComponent = Verhalten**; einfach wiederverwendbar/erweiterbar.

---

## Inventory & Storage (FastArray, Multi-User)

### `URPGInventoryComponent`
- Datenhaltung: `TArray<FInventoryEntry>` (FastArray).
- **API (C++-First):** Public C++-Methoden; **nur das Nötigste BlueprintCallable** (UI öffnen/schließen, reine View-Events).
- Kernfunktionen:
    - `AddItem`, `RemoveItem`, `MoveItem`, `SplitStack`, `MergeStack`
    - `Consume`, `Equip`, `Unequip`
    - `DropToWorld` (spawnt `ARPGWorldPickup` serverseitig)
- Validierung (Server): MaxStack, Slot-Kompatibilität, Weight/Capacity, Permissions.
- **Nicht am Pawn hängen** → PlayerState-gebundenes Owner-Objekt/Component, damit bei Pawn-Wechsel persistent.

### `URPGStorageContainerComponent`
- Gleiches Datenmodell wie Inventory.
- **Multi-User gleichzeitig**:
    - Server-autoritative **Sessions** pro Container (SessionId/Revision).
    - Optimistische Client-Operationen → Server validiert, sendet Delta (FastArray).
    - Konflikte gelöst auf Server (First-Commit-Wins je Slot; Clients erhalten Delta/Toast).
- Transfer-APIs: `TransferToContainer`, `TransferFromContainer`, `TransferBetween`.

---

## GAS-Integration
- **Consumables** → `ApplyGameplayEffectToOwner` oder `TryActivateAbility` (aus Fragment-Daten).
- **Equippables** → Grants/Removals via ASC (Abilities, AttributeSets/Mods), Slot-Tags prüfen.
- **Cooldowns**/Gates via GameplayTags (z. B. `State.Cooldown.Potion`).

---

## GameplayTags (Native in C++)
- Native-Registrierung in C++:
  ```cpp
  UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot);
  UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Weapon_MainHand);
  UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Weapon_OffHand);
  UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Armor_Head);
  UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Armor_Body);

  UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Consumable);
  UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Weapon);

  UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Cooldown_Potion);
