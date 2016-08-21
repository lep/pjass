function x takes integer y returns nothing
    // a local declaration puts the name into scope directly
    // and not after the assignment is evaluated
    // so the expression y refers to the just introduced local and
    // not the parameter and this throws an uninititalized error
    local integer y = y
endfunction
