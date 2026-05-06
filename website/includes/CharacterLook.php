<?php
/**
 * PKO Character Look Parser
 * 
 * Decodes the 'look' field from the character table to extract appearance data.
 * 
 * Look Format (v112):
 *   "112#TypeID,HairID;EquipSlot0Data;EquipSlot1Data;...;Checksum"
 * 
 * Equipment Slots (enumEQUIP_NUM = 10):
 *   0 = Weapon (Right Hand)
 *   1 = Shield/Offhand (Left Hand)
 *   2 = Head/Hat
 *   3 = Chest/Armor
 *   4 = Gloves
 *   5 = Boots
 *   6 = Accessory 1
 *   7 = Accessory 2  
 *   8 = Accessory 3
 *   9 = Fashion/Costume
 */

class CharacterLook
{
    // Equipment slot constants
    const SLOT_WEAPON = 0;
    const SLOT_SHIELD = 1;
    const SLOT_HEAD = 2;
    const SLOT_CHEST = 3;
    const SLOT_GLOVES = 4;
    const SLOT_BOOTS = 5;
    const SLOT_ACCESSORY1 = 6;
    const SLOT_ACCESSORY2 = 7;
    const SLOT_ACCESSORY3 = 8;
    const SLOT_FASHION = 9;
    const EQUIP_SLOT_COUNT = 10;
    
    // Character type IDs (from CharacterInfo.txt)
    const TYPE_LANCE = 1;      // Male Lance (Swordsman)
    const TYPE_CARSISE = 2;    // Male Carsise (Hunter)
    const TYPE_PHYLLIS = 3;    // Female Phyllis (Magic)
    const TYPE_AMI = 4;        // Female Ami (Support)
    
    // Equipment slot names
    public static $slotNames = [
        self::SLOT_WEAPON => 'Weapon',
        self::SLOT_SHIELD => 'Shield',
        self::SLOT_HEAD => 'Head',
        self::SLOT_CHEST => 'Chest',
        self::SLOT_GLOVES => 'Gloves',
        self::SLOT_BOOTS => 'Boots',
        self::SLOT_ACCESSORY1 => 'Accessory',
        self::SLOT_ACCESSORY2 => 'Accessory',
        self::SLOT_ACCESSORY3 => 'Accessory',
        self::SLOT_FASHION => 'Fashion',
    ];

    // Character type names
    public static $typeNames = [
        self::TYPE_LANCE => 'Lance',
        self::TYPE_CARSISE => 'Carsise',
        self::TYPE_PHYLLIS => 'Phyllis',
        self::TYPE_AMI => 'Ami',
    ];
    
    public int $version = 0;
    public int $typeId = 0;
    public int $hairId = 0;
    public array $equipment = [];
    public bool $isValid = false;
    public string $error = '';
    
    /**
     * Parse the look field from database
     */
    public static function parse(?string $lookData): self
    {
        $look = new self();
        
        if (empty($lookData)) {
            $look->error = 'Empty look data';
            return $look;
        }
        
        try {
            // Check for version header (e.g., "112#")
            $parts = explode('#', $lookData, 2);
            
            if (count($parts) === 2) {
                $look->version = (int)$parts[0];
                $data = $parts[1];
            } else {
                $look->version = 0; // Old format
                $data = $lookData;
            }
            
            // Split by semicolon to get sections
            $sections = explode(';', $data);
            
            if (count($sections) < 2) {
                $look->error = 'Invalid look format: too few sections';
                return $look;
            }
            
            // First section: TypeID,HairID
            $typeHair = explode(',', $sections[0]);
            if (count($typeHair) >= 2) {
                $look->typeId = (int)$typeHair[0];
                $look->hairId = (int)$typeHair[1];
            }
            
            // Equipment sections (slots 0-9)
            for ($i = 0; $i < self::EQUIP_SLOT_COUNT && $i < count($sections) - 1; $i++) {
                $slotData = $sections[$i + 1];
                $look->equipment[$i] = self::parseEquipmentSlot($slotData, $look->version);
            }
            
            $look->isValid = true;
            
        } catch (Exception $e) {
            $look->error = 'Parse error: ' . $e->getMessage();
        }
        
        return $look;
    }
    
    /**
     * Parse a single equipment slot data
     */
    private static function parseEquipmentSlot(string $slotData, int $version): array
    {
        $parts = explode(',', $slotData);
        $slot = [
            'itemId' => 0,
            'quantity' => 0,
            'endure' => [0, 0],
            'energy' => [0, 0],
            'forgeLevel' => 0,
            'dbParams' => [],
            'instanceAttrs' => [],
        ];
        
        $index = 0;
        
        // Version 112 format has extra fields at the beginning
        if ($version >= 112) {
            // expiration, bItemTradable, bIsLock, sNeedLv, dwDBID
            if (isset($parts[$index])) $slot['expiration'] = (int)$parts[$index++];
            if (isset($parts[$index])) $slot['tradable'] = (int)$parts[$index++];
            if (isset($parts[$index])) $slot['locked'] = (int)$parts[$index++];
            if (isset($parts[$index])) $slot['requiredLevel'] = (int)$parts[$index++];
            if (isset($parts[$index])) $slot['dbId'] = (int)$parts[$index++];
        }
        
        // Core item data
        if (isset($parts[$index])) $slot['itemId'] = (int)$parts[$index++];
        if (isset($parts[$index])) $slot['quantity'] = (int)$parts[$index++];
        if (isset($parts[$index])) $slot['endure'][0] = (int)$parts[$index++];
        if (isset($parts[$index])) $slot['endure'][1] = (int)$parts[$index++];
        if (isset($parts[$index])) $slot['energy'][0] = (int)$parts[$index++];
        if (isset($parts[$index])) $slot['energy'][1] = (int)$parts[$index++];
        if (isset($parts[$index])) $slot['forgeLevel'] = (int)$parts[$index++];
        
        return $slot;
    }
    
    /**
     * Get the character type name
     */
    public function getTypeName(): string
    {
        return self::$typeNames[$this->typeId] ?? 'Unknown';
    }
    
    /**
     * Get the gender based on character type
     */
    public function getGender(): string
    {
        return in_array($this->typeId, [self::TYPE_LANCE, self::TYPE_CARSISE]) ? 'Male' : 'Female';
    }
    
    /**
     * Check if an equipment slot has an item
     */
    public function hasEquipment(int $slot): bool
    {
        return isset($this->equipment[$slot]) && $this->equipment[$slot]['itemId'] > 0;
    }
    
    /**
     * Get equipment item ID for a slot
     */
    public function getEquipmentId(int $slot): int
    {
        return $this->equipment[$slot]['itemId'] ?? 0;
    }
    
    /**
     * Get all equipped item IDs
     */
    public function getEquippedItemIds(): array
    {
        $ids = [];
        foreach ($this->equipment as $slot => $item) {
            if ($item['itemId'] > 0) {
                $ids[$slot] = $item['itemId'];
            }
        }
        return $ids;
    }
    
    /**
     * Get the texture file path for the character's base appearance
     * 
     * Texture naming convention:
     * - Base character: {typeId}000000{variant}.bmp
     * - Hair: Encoded in hairId
     * - Equipment: Based on itemId mapping to texture files
     */
    public function getBaseTextureId(): string
    {
        // Format: TTTT000000.bmp where TTTT is padded type ID
        return sprintf('%04d%06d', $this->typeId, 0);
    }
    
    /**
     * Get character appearance summary for display
     */
    public function getSummary(): array
    {
        $summary = [
            'type' => $this->getTypeName(),
            'gender' => $this->getGender(),
            'hair' => $this->hairId,
            'equipment' => [],
        ];
        
        foreach (self::$slotNames as $slot => $name) {
            if ($this->hasEquipment($slot)) {
                $summary['equipment'][$name] = [
                    'id' => $this->equipment[$slot]['itemId'],
                    'forge' => $this->equipment[$slot]['forgeLevel'],
                ];
            }
        }
        
        return $summary;
    }
    
    /**
     * Generate a simple avatar identifier for CSS-based rendering
     * This returns data needed to render a paper-doll style character
     */
    public function getAvatarData(): array
    {
        return [
            'typeId' => $this->typeId,
            'typeName' => $this->getTypeName(),
            'gender' => $this->getGender(),
            'hairId' => $this->hairId,
            'weapon' => $this->getEquipmentId(self::SLOT_WEAPON),
            'shield' => $this->getEquipmentId(self::SLOT_SHIELD),
            'head' => $this->getEquipmentId(self::SLOT_HEAD),
            'chest' => $this->getEquipmentId(self::SLOT_CHEST),
            'gloves' => $this->getEquipmentId(self::SLOT_GLOVES),
            'boots' => $this->getEquipmentId(self::SLOT_BOOTS),
            'fashion' => $this->getEquipmentId(self::SLOT_FASHION),
        ];
    }
}
