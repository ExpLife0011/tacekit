/**
 * override.h
 * Overrides the return value of a function on the call stack.
 *
 * © 2018 fereh
 */

#pragma once
#include <windef.h>

void __inline OverrideReturn(void *newValue, DWORD_PTR basePtr);