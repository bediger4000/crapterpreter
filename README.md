# crapterpreter
An unremarkable interpreted programming language.

The following implementation of a factorial program
shows off many of the unremarkable features. It can
recurse, it has functions, it has while-loops and an
output operator, "->".

    fun fact(x) {
    	if (x == 1) {
    		r = 1;
    	} else {
    		z = x - 1;
    		m = fact(z);
    		r = x * m;
    	}
    	return r;
    }
    run;
    print fact;
    n = 5;
    while (n > 0) {
    	x = fact(n);
    	-> "fact(",n,") = ", x;
    	n = n - 1;
    }
    run;
