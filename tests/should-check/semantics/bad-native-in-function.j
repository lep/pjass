native UnitId2String takes integer id returns string

// Some natives are reported when used in a globals block. This check is to
// make sure that they are not reported anywhere else.
function foo takes nothing returns string
    return UnitId2String('hpea')
endfunction
