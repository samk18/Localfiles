def solution(n, K):
    # write your code in Python 3.6
    bigdig=""
    digit=str(n)
    Nine="9"
    for j in range(len(digit)):
        diff=9-int(digit[j])
        if diff>=K:
            bigdig=bigdig+str(int(digit[j])+K)+digit[j+1:]
            return int(st)
        else:
            K=K-diff
            st=st+Nine
            
    return int(st) 
    

print(solution(101,5))