globals
    string O
    integer T
endglobals
//+rb
function b takes nothing returns nothing
call error_on_line_7()
// 0x13
set T = ''
// 0x10
set T = '
'
// 0x13
set O = ""
// 0x10
set O = "
"
// 0x1013
set O = "
"
call error_on_line_23()
endfunction