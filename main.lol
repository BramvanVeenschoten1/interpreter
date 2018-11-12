x = {
	1: "lala", 
	2: 36,
	3: 3.1415,
	4: [1,2,3],
};

print(x);

pop(x, 2);

print(x);

factorial = (x){
	crutch = (x, y){
		equals(y, 1)(
			true,
			crutch,
		)( mul(x, y), sub(y, 1) ); 
	};
	crutch(1, x);
};

print(factorial(6));

for = (func, count){
	func(count);
	equals(count, 0)( (x, y){}, for)( func, sub(count, 1) );
};

list = [];
push = (x){
	add(list, x);
};

for(push, 10);

print(list);

z = {{{{{}:0}:0}:0}:0};

print(z);