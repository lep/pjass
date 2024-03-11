native StringHash takes string s returns integer

function foo takes nothing returns nothing
    // we just test various (wrong) calls to StringHash to make sure we don't
    // hit any fatals in our check. this is more of a sanity check for the
    // C code than a check of pjass functionality.
    call StringHash()
    call StringHash("a", "b", "c")
    call StringHash(123)
endfunction