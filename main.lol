factorial = (x){
	crutch = (x, y){
		equals(y, 1)(
			true,
			crutch,
		)( mul(x, y), sub(y, 1) ); 
	};
	crutch(1, x);
};

print([1, 2, 3, 4, 54, 5, 65, 6, 7, 7,8 , 9, 9]);

print(factorial(6));

square = (x){
	helper = (a, b, c){
		equals(x, a)(
			(p, q, r){q},
			helper,
		)(add(a, 1), add(b, c), add(c, 2));
	};
	helper(1, 0, 1);
};

print(square(9));

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