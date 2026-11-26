/* SPDX-License-Identifier: GPL-3.0-or-later
 * Produces an import library only. The DLL is NEVER loaded by the experiment. */
__declspec(dllexport) int fixture_add(int a,int b) { return a+b; }
