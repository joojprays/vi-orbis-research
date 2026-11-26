/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original synthetic fixture. No game or console code. */
__declspec(dllimport) int fixture_add(int a,int b);
volatile int fixture_value=19;
volatile int *volatile fixture_pointer=&fixture_value;
int lab_entry(void) { return fixture_add(*fixture_pointer,23); }
