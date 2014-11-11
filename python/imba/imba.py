#!/usr/bin/python2
#
# imba.py: Compute the minimal imbalance of the binary tree with a set
#    of leaves of a given weight.
#
# James Sullivan

import sys

def print_matrix(matr):
    for row in matr:
        print"[",
        for ent in row:
            print"%2.3f "%ent,
        print("]")

# Compute the imbalance for the subtree containing elements i through j,
#  splitted at element k.
def imbal(lst, i, j, k):
    l = sum(lst[i:j+1])
    r = sum(lst[j+1:k+1])
    if(l == 0 or r == 0):
        return 0
    return max(float(l)/float(r), float(r)/float(l))

# Returns the matrix containing all minimal imbalances for all sub-trees
def find_imbalances(lst):
    if(len(lst) <= 1):
        return None
    imbalances = [[0 for col in range(0,len(lst))] for row in
            range(0,len(lst))]
    weights = [[0 for col in range(0,len(lst))] for row in
            range(0,len(lst))]

    # Initialize a single diagonal
    for i in range(0,len(lst)-1):
        r = float(lst[i+1])/float(lst[i])
        imbalances[i][i+1] = max(r, 1/r)

    # Fill the matrix bottom-up
    for r in range((len(lst)-3), -1, -1):
        # Fill the matrix left to right
        for c in range(r+2, len(lst)):
            # Find the optimal split for the subtree containing
            # elements r through c
            imb = 1000000
            for k in range(r,c):
                # Look up optimal left- and right-subtree values
                lImb = imbalances[r][k]
                rImb = imbalances[k+1][c]
                # Compute the imbalance of the split itself
                tImb = imbal(lst,r,k,c)
                # Compute the max of all of these imbalances
                thisImb = max(tImb, lImb)
                thisImb = max(thisImb, rImb)
                # Set the new minimum value
                imb = min(thisImb, imb)
            imbalances[r][c] = imb

    return imbalances

def main():
    n = len(sys.argv) - 1
    lst = []
    for i in range(1,n+1):
        lst.append(int(sys.argv[i]))
    print "Input:", lst
    imbalances = find_imbalances(lst)
    if(imbalances == None):
        print "No solution"
        return -1
    print_matrix(imbalances)

    maxr = imbalances[0][n-1]
    print "Result:", maxr
    return maxr

if __name__ == "__main__":
    main()
