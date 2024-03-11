
// In previous patches you could `return null` in a function which returned
// code. At the time of writing this is not the case anymore. But you can still
// use null in code assignment.
function foo takes nothing returns nothing
    local code c = null
endfunction
