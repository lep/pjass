native StringHash takes string s returns integer

//# +checkstringhash
function bla takes nothing returns nothing
    call StringHash("foo")
    call StringHash("Foo")
endfunction