--[[ This function is executed every time you press the 'execute' button ]]
function init()
end


--[[ This function is executed at each time step
     It must contain the logic of your controller ]]
function step()
   for index, device in ipairs(robot.rangefinders) do
      local message = 
         string.format('%d: distance = %.2f, illuminance = %.2f', index,
                       device.reading.proximity, device.reading.illuminance)
      log(message)
   end
   log('---')
end

--[[ This function is executed every time you press the 'reset'
     button in the GUI. It is supposed to restore the state
     of the controller to whatever it was right after init() was
     called. The state of sensors and actuators is reset
     automatically by ARGoS. ]]
function reset()
end

--[[ This function is executed only once, when the robot is removed
     from the simulation ]]
function destroy()
   -- put your code here
end
