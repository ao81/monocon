struct common {
	int SDI : 1;
	int SCK : 1;
	int LAT1 : 1;
	int LAT2 : 1;
	int DCM : 2;
	int BZ : 1;
	int TSW : 1;
};

struct bitset RC;

union {
	struct {
		int SM : 4;
		int R : 1;
		int B : 1;
		int G : 1;
		int res : 1;
	} bit;
	int b8;
} lm;
