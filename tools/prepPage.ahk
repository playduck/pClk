#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.


IfExist, %1%
{
	msgbox %1%
	Input = %1%
	goto ButtonExecute
}

Gui, Add, Text,, Input:
Gui, Add, Edit, vInput w350 h200
Gui, Add, Button, default, Execute

Gui, Show,, EmbedPage
return

ButtonExecute:
Gui, Submit
Gui, Destroy

StringReplace, Input, Input, %A_Tab%, %A_Space%, All

Loop
{
    StringReplace, Input, Input, `r`n,, UseErrorLevel
	StringReplace, Input, Input, `r,, UseErrorLevel
	StringReplace, Input, Input, `n,, UseErrorLevel
    if ErrorLevel = 0  ; No more replacements needed.
        break
}

StringReplace, Output, Input, ", \",, All

Gui, Add, Text,, Output:
Gui, Add, Edit, w350 h200, %Output% 
Gui, Add, Button, default, Copy
Gui, Add, Button,, Close

Gui, Show,, EmbedPage
return

ButtonCopy:
clipboard = %Output%

GuiClose:
ButtonClose:
ExitApp