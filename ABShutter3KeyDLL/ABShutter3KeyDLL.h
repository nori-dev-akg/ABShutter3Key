/*
Code copyright 2020 nori-dev-akg
Code released under the MIT license
https://opensource.org/licenses/mit-license.php
*/
#pragma once
#ifdef ABSHUTTER3KEYDLL_EXPORTS
#define HOOKINGRAWINPUTDEMODLL_API __declspec(dllexport)
#else
#define HOOKINGRAWINPUTDEMODLL_API __declspec(dllimport)
#endif

HOOKINGRAWINPUTDEMODLL_API BOOL InstallHook(HWND hwndParent);

HOOKINGRAWINPUTDEMODLL_API BOOL UninstallHook();
