//# +rb
type foo extends integer
//# +rb

// this does not throw an error
//# +rb 
function bar takes handle h returns integer //# some comment
	return h
	return 0
endfunction
//# +rb 