#include <exception.h>

String myString = (String)"classy string\0";
//string newString = myString;

String myutf8 = "!!! Schöne Grüße 😊 !!!";
const char blah[] = " - animal";
String pliny[3] = {"cats", "dogs", "bummer"};

struct _DICT {
	struct _DICT *this;
	int count;
	int find;
	String blah[10];

	/*int go() {
		//printf("this find = %d\n", this.find);
		return this.find;
	};*/
};
typedef struct _DICT DICT;

class _myClass {
	int find;
	String hello;
	class _myClass *this;

//	int go() {
//		//printf("this find = %d\n", this.find);
//		return this.find;
//	};
};

typedef class _myClass myClass;


//DICT x = {5, (String)"hey bey"};

//const char *const_char = "const char string";

int nums = 1;
char ss[] = "char string array";

void main() {
	Exception e;
	String search = "Hello, this is a test";

	printf("%s \n", search);

	Try {
		myClass snatch;
		snatch.find = -10;
		snatch.hello = "Snarf snarf!";
		snatch.this = &snatch;
		snatch.this->find = 10;

		//snatch.go();
		printf("snatch this=%p find = [%d] hello = \"%s\"\n", snatch.this, snatch.find, snatch.hello);
	
		Throw(-1, "Bad pie");
	} Catch(e) {
		printf("Exception: %s\n", e.msg);
	}


	//DICT mydict;
	//mydict.count = -2;
	//mydict.blah[0] = "mydict";
	//snatch.go();

	//int x = go();

	printf("Hello %s\n", myutf8 + pliny[0]);
	//printf("DICT %d - %s\n", mydict.count, mydict.blah[0]);
}
