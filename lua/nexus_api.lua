-- Nexus Game Engine Lua API
-- High-level Lua interfaces for the Nexus Game Engine

-- Engine constants
NEXUS_VERSION = "1.0.0"

-- Key codes
Keys = {
    ESC = 27,
    SPACE = 32,
    LEFT = 37,
    UP = 38,
    RIGHT = 39,
    DOWN = 40,
    A = string.byte('A'),
    B = string.byte('B'),
    C = string.byte('C'),
    D = string.byte('D'),
    E = string.byte('E'),
    F = string.byte('F'),
    G = string.byte('G'),
    H = string.byte('H'),
    I = string.byte('I'),
    J = string.byte('J'),
    K = string.byte('K'),
    L = string.byte('L'),
    M = string.byte('M'),
    N = string.byte('N'),
    O = string.byte('O'),
    P = string.byte('P'),
    Q = string.byte('Q'),
    R = string.byte('R'),
    S = string.byte('S'),
    T = string.byte('T'),
    U = string.byte('U'),
    V = string.byte('V'),
    W = string.byte('W'),
    X = string.byte('X'),
    Y = string.byte('Y'),
    Z = string.byte('Z')
}

-- Mouse buttons
Mouse = {
    LEFT = 1,
    RIGHT = 2,
    MIDDLE = 3
}

-- Color utilities
Color = {
    BLACK = {0, 0, 0, 1},
    WHITE = {1, 1, 1, 1},
    RED = {1, 0, 0, 1},
    GREEN = {0, 1, 0, 1},
    BLUE = {0, 0, 1, 1},
    YELLOW = {1, 1, 0, 1},
    MAGENTA = {1, 0, 1, 1},
    CYAN = {0, 1, 1, 1},
    GRAY = {0.5, 0.5, 0.5, 1}
}

-- Vector2 class
Vector2 = {}
Vector2.__index = Vector2

function Vector2.new(x, y)
    local v = {x = x or 0, y = y or 0}
    setmetatable(v, Vector2)
    return v
end

function Vector2:length()
    return math.sqrt(self.x * self.x + self.y * self.y)
end

function Vector2:normalize()
    local len = self:length()
    if len > 0 then
        self.x = self.x / len
        self.y = self.y / len
    end
    return self
end

function Vector2:add(other)
    return Vector2.new(self.x + other.x, self.y + other.y)
end

function Vector2:subtract(other)
    return Vector2.new(self.x - other.x, self.y - other.y)
end

function Vector2:multiply(scalar)
    return Vector2.new(self.x * scalar, self.y * scalar)
end

-- Vector3 class
Vector3 = {}
Vector3.__index = Vector3

function Vector3.new(x, y, z)
    local v = {x = x or 0, y = y or 0, z = z or 0}
    setmetatable(v, Vector3)
    return v
end

function Vector3:length()
    return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)
end

function Vector3:normalize()
    local len = self:length()
    if len > 0 then
        self.x = self.x / len
        self.y = self.y / len
        self.z = self.z / len
    end
    return self
end

function Vector3:add(other)
    return Vector3.new(self.x + other.x, self.y + other.y, self.z + other.z)
end

function Vector3:subtract(other)
    return Vector3.new(self.x - other.x, self.y - other.y, self.z - other.z)
end

function Vector3:multiply(scalar)
    return Vector3.new(self.x * scalar, self.y * scalar, self.z * scalar)
end

function Vector3:dot(other)
    return self.x * other.x + self.y * other.y + self.z * other.z
end

function Vector3:cross(other)
    return Vector3.new(
        self.y * other.z - self.z * other.y,
        self.z * other.x - self.x * other.z,
        self.x * other.y - self.y * other.x
    )
end

-- GameObject class
GameObject = {}
GameObject.__index = GameObject

function GameObject.new(x, y, z)
    local obj = {
        position = Vector3.new(x, y, z),
        velocity = Vector3.new(0, 0, 0),
        rotation = Vector3.new(0, 0, 0),
        scale = Vector3.new(1, 1, 1),
        active = true
    }
    setmetatable(obj, GameObject)
    return obj
end

function GameObject:update(deltaTime)
    if self.active then
        self.position = self.position:add(self.velocity:multiply(deltaTime))
    end
end

function GameObject:setPosition(x, y, z)
    self.position.x = x
    self.position.y = y
    self.position.z = z
end

function GameObject:getPosition()
    return self.position.x, self.position.y, self.position.z
end

function GameObject:setVelocity(x, y, z)
    self.velocity.x = x
    self.velocity.y = y
    self.velocity.z = z
end

-- Game class template
Game = {}
Game.__index = Game

function Game.new()
    local game = {
        objects = {},
        running = true,
        initialized = false
    }
    setmetatable(game, Game)
    return game
end

function Game:initialize()
    log_info("Game initialized in Lua!")
    self.initialized = true
    return true
end

function Game:update(deltaTime)
    if not self.initialized then return end
    
    -- Update all game objects
    for _, obj in ipairs(self.objects) do
        obj:update(deltaTime)
    end
    
    -- Check for exit
    if is_key_pressed(Keys.ESC) then
        self.running = false
    end
end

function Game:render()
    if not self.initialized then return end
    
    -- Clear screen with blue background
    clear_screen(0.2, 0.3, 0.8, 1.0)
    
    -- Render game objects here
    
    -- Present frame
    present_frame()
end

function Game:shutdown()
    log_info("Game shutdown in Lua!")
    self.initialized = false
end

function Game:addObject(obj)
    table.insert(self.objects, obj)
end

function Game:removeObject(obj)
    for i, o in ipairs(self.objects) do
        if o == obj then
            table.remove(self.objects, i)
            break
        end
    end
end

-- Utility functions
function lerp(a, b, t)
    return a + (b - a) * t
end

function clamp(value, min, max)
    if value < min then return min end
    if value > max then return max end
    return value
end

function distance(x1, y1, x2, y2)
    local dx = x2 - x1
    local dy = y2 - y1
    return math.sqrt(dx * dx + dy * dy)
end

function angle(x1, y1, x2, y2)
    return math.atan2(y2 - y1, x2 - x1)
end

-- Print welcome message
log_info("Nexus Lua API loaded successfully!")