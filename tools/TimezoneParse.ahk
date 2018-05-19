#NoEnv  ; Recommended for performance and compatibility with future AutoHotkey releases.
; #Warn  ; Enable warnings to assist with detecting common errors.
SendMode Input  ; Recommended for new scripts due to its superior speed and reliability.
SetWorkingDir %A_ScriptDir%  ; Ensures a consistent starting directory.

FileRead, f, data/timezone.csv

f := RegExReplace(f, "\n", ",")

f := RegExReplace(f, "/", " / ")
f := RegExReplace(f, "_", " ")

first = 1
curr = ""

Loop, PARSE, f, csv
{
	a := Mod(A_index, 2)

	;;Msgbox, %a% %A_index% %A_LoopField%

	if a = 1
	{
		c = %A_LoopField%</option>
		d := RegExReplace(c, ".*[?=\/]\s", "")
		if (first = 1 or InStr(c, curr) = 0 )
		{
			if( first = 1 )	
			{
			first = 0
			Goto, EL
			}
			curr := RegExReplace(c, "[?=\/].*", "" )
			d = %d%`r
		}
		Goto, EL
	}
	if a = 0
	{
		b = <option value="%A_LoopField%">
	}

	int := b . d
	out := out . int
EL:
}

clipboard =%out%
Msgbox, %out%
ExitApp

^c::
	ExitApp, 1