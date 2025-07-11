-- Example Lua game for Nexus Engine
require("nexus_api")

-- Game state
local player = GameObject.new(0, 0, 0)
local enemies = {}
local bullets = {}
local score = 0
local gameTime = 0

-- Game settings
local PLAYER_SPEED = 200
local BULLET_SPEED = 300
local ENEMY_SPEED = 100
local ENEMY_SPAWN_TIME = 2.0
local lastEnemySpawn = 0

function initialize()
    -- filepath: g:\code\gameengine\lua\example_game.lua
-- Example Lua game for Nexus Engine
require("nexus_api")

-- Game state
local player = GameObject.new(0, 0, 0)
local enemies = {}
local bullets = {}
local score = 0
local gameTime = 0

-- Game settings
local PLAYER_SPEED = 200
local BULLET_SPEED = 300
local ENEMY_SPEED = 100
local ENEMY_SPAWN_TIME = 2.0
local lastEnemySpawn =