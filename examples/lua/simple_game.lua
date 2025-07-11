-- Simple Lua game using Nexus Engine

-- Game state
local player = {
    x = 0,
    y = 0,
    z = 0,
    rotation = 0
}

local score = 0
local gameTime = 0

-- Game update function
function update(deltaTime)
    gameTime = gameTime + deltaTime
    
    -- Simple player movement
    if input_is_key_down(string.byte('W')) then
        player.z = player.z + 5 * deltaTime
    end
    if input_is_key_down(string.byte('S')) then
        player.z = player.z - 5 * deltaTime
    end
    if input_is_key_down(string.byte('A')) then
        player.x = player.x - 5 * deltaTime
    end
    if input_is_key_down(string.byte('D')) then
        player.x = player.x + 5 * deltaTime
    end
    
    -- Rotate player
    player.rotation = player.rotation + deltaTime
    
    -- Increase score over time
    score = score + math.floor(deltaTime * 10)
end

-- Game render function
function render()
    -- Clear screen with blue color
    graphics_clear(0.2, 0.3, 0.8, 1.0)
    
    -- Draw player cube
    graphics_draw_cube(player.x, player.y, player.z, 1.0, 1.0, 1.0, 1.0, 0.5, 0.0, 1.0)
    
    -- Draw ground tiles
    for i = -10, 10, 2 do
        for j = -10, 10, 2 do
            if i ~= 0 or j ~= 0 then
                graphics_draw_cube(i, -1, j, 0.5, 0.1, 0.5, 0.3, 0.7, 0.3, 1.0)
            end
        end
    end
    
    print("Score: " .. score)
    print("Time: " .. string.format("%.1f", gameTime))
end

-- Initialize game
function init()
    print("Lua Game Initialized!")
    print("Use WASD to move, ESC to quit")
end

-- Game loop hook
function game_loop()
    -- This function is called every frame
    local deltaTime = engine_get_delta_time()
    
    update(deltaTime)
    
    graphics_begin_frame()
    render()
    graphics_end_frame()
end

-- Start the game
init()
print("Starting Lua game...")