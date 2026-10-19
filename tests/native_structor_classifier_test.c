#include "native_structor_adapter.h"

#include <stdio.h>

void machgate_native_structor_adapter_entry(void)
{
}

struct classifier_case {
	const char* symbol;
	int expected;
};

int main(void)
{
	static const struct classifier_case cases[] = {
		{"__ZN3foo3BarC1Ev", 1},
		{"_ZN3foo3BarIiEC2Ev", 1},
		{"_ZN1AILi123EEC1Ev", 1},
		{"_ZN1AIXadL_Z5InnervEEEC1Ev", 1},
		{"_ZN1BCI11AEi", 1},
		{"_ZN1BCI21AEi", 1},
		{"_ZN3foo3BarD1Ev", 1},
		{"_ZN3foo3BarD2Ev", 1},
		{"_ZN3foo3C1EEv", 0},
		{"_ZTv0_n24_N3foo3BarD1Ev", 0},
		{"_ZN3foo3BarD0Ev", 0},
		{"_ZN3foo3BarC3Ev", 0},
		{"_ZN3foo3BarC5Ev", 0},
		{"_ZN3foo3BarD5Ev", 0},
	};

	for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]);
	     index++) {
		int result = machgate_native_symbol_has_structor_return(
			cases[index].symbol);
		if (result == cases[index].expected)
			continue;
		fprintf(stderr, "classifier mismatch for %s: got %d, expected %d\n",
		        cases[index].symbol, result, cases[index].expected);
		return 1;
	}
	return 0;
}
