
type agent extends handle
type widget extends agent
type unit extends widget
type destructable extends widget

globals
    unit b
endglobals

function A takes nothing returns nothing
    set b = null
endfunction

function B takes nothing returns nothing
    local destructable b
endfunction

function C takes nothing returns nothing
    set b = null
endfunction
