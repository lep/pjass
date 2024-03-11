globals
	integer x
endglobals

//# +checkglobalsinit
function foo takes nothing returns integer
	set x = 3
	return x
endfunction
	
