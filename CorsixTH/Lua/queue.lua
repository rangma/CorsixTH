--[[ Copyright (c) 2009 Peter "Corsix" Cawley

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. --]]

class "Queue"
-- A queue stores a list of humanoids waiting to use an object.
-- For a reception desk, this is as expected.
-- For a room, the queue is for the door, not the room. Hence the queue stores
-- the list of patients waiting to enter (the traditional queue for the room),
-- the list of staff waiting to enter (because of the door being in use for
-- example), the list of staff and patients waiting to leave (again due to door
-- being in use).

function Queue:Queue()
  self.expected = 0
  self.visitor_count = 0
  self.max_size = 20
end

function Queue:decrease_max_size()
  if self.max_size > 0 then
    self.max_size = self.max_size - 1
  end
end

function Queue:increase_max_size()
  if self.max_size < 30 then
    self.max_size = self.max_size + 1
  end
end

function Queue:size()
  return #self
end

function Queue:setPriorityForSameRoom(entity)
  self.same_room_priority = entity
end

function Queue:push(humanoid)
  local index = #self + 1
  if self.same_room_priority then
    -- If humanoid in the priority room, then position them in the queue before
    -- humanoids not in the room (because if they are in the room and in the
    -- queue, then they are trying to leave the room).
    local room = self.same_room_priority:getRoom()
    if humanoid:getRoom() == room then
      while index > 1 do
        local before = self[index - 1]
        if before:getRoom() == room then
          break
        end
        index = index - 1
      end
    end
  end
  if class.is(humanoid, Staff) then
    -- Give staff priority over patients
    while index > 1 do
      local before = self[index - 1]
      if class.is(before, Staff) then
        break
      end
      index = index - 1
    end
  end
  table.insert(self, index, humanoid)
end

function Queue:front()
  return self[1]
end

function Queue:pop()
  for i = #self, 2, -1 do
    local humanoid = self[i]
    if humanoid.onAdvanceQueue then
      humanoid:onAdvanceQueue(self, i - 1)
    end
  end
  
  local oldfront = self[1]
  table.remove(self, 1)
  if oldfront.onLeaveQueue then
    oldfront:onLeaveQueue(self)
  end
  return oldfront
end

function Queue:remove(index)
  if self[index] == nil then
    return
  end
  local value = self[index]
  for i = #self, index + 1, -1 do
    local humanoid = self[i]
    if humanoid.onAdvanceQueue then
      humanoid:onAdvanceQueue(self, i - 1)
    end
  end
  table.remove(self, index)
  return value
end

function Queue:removeValue(value)
  for i = 1, #self do
    if self[i] == value then
      self:remove(i)
      return true
    end
  end
  return false
end