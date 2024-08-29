
def minCntCharDeletionsfrequency(a):
	

	frequency = {}
	pq = []

	cntChar = 0

	for i in a:
        if not(i in frequency):
              frequency

	for it in mp:
		pq.append(mp[it])

	pq = sorted(pq)

	while (len(pq) > 0):
		
		frequent = pq[-1]

		del pq[-1]

		if (len(pq) == 0):

			return cntChar

		if (frequent == pq[-1]):
			

			if (frequent > 1):

				pq.append(frequent - 1)

			cntChar += 1
			
		pq = sorted(pq)
		
	return cntChar

if __name__ == '__main__':
	
	str = "abbbcccd"

	N = len(str)
	
	print(minCntCharDeletionsfrequency(str, N))


git stash drop stash@{n}