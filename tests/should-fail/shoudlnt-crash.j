// this file used to crash pjass
// found by afl (this is a reduced file though)

//# +rb
function bar takes handle h returns integer
return 0
type foo extends integer
endfunction