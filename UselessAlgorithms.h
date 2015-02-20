// Sieve of Eratosthenes
inline vector<int> generatePrimes(int n)
{
	// number of composites in this numeric base system
	vector<int> primes;
	
	// we can stop after ceil(n^(1/2)) as every numbers highest composite is its squareroot.
	long root = (long)(ceil(sqrt(n)));
	
	// create a clear list (initialized to false)
	auto markedList = std::vector<bool>(n);
	
	// mark everything not prime in the list.
	// we start at two (index 3). primes are numbers that satisfy (prime % n != 0, where n != prime, n != 0, n != 1)
	// we include the squareroot of n as well.
	// note that is possible to skip even numbers and increment by 2, because they are per definition multiples of 2,
	// but that would need a special case-check in the start
	
	for(int i = 2; i <= root; i++)
	{
		// is table not yet marked?
		if(!markedList[i])
		{
			// we keep this index non-marked - thus it is a prime, because it hasn't been marked yet.
			// mark the next square of i and all successive multiples of i as well as non-primes
			for(int j = i * i; j < n; j += i)
			{
				// mark this as not prime (because it is a multiple of i)
				markedList[j] = true;
			}
		}
	}
	
	// okay, so we marked all non-primes.
	// now we just return all indexes that are not marked!
	// we start at 2 because index 1 == the second prime (so we exclude 0 and 1 as a prime)
	for(int i = 2; i < n; ++i)
	{
		if(!markedList[i])
		{
			primes.push_back(i);
		}
	}
	return primes;
}
