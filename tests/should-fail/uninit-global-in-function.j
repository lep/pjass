globals
	integer x
endglobals

//# +checkglobalsinit
function foo takes nothing returns nothing
	local integer y = x
endfunction
