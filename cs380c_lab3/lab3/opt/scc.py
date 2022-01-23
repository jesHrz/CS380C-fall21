import sys

# Problem description: Find the strongly connected components in a graph

# Input:
# Each test file consists of m+1 lines.
# The first line contains two integers n and m, separated by a space.
# As usual, n is the total number of nodes in the graph and m is the
# total number of edges. Nodes are indexed from 1 to n.
# Each of the next m lines contains two integers u and v separated by a
# space, and indicating a directed edge from node u to node v.

# Output:
# Your output file should consist of K+1 lines, where K is the number
# of strongly connected components in the input graph.
# The first line in your output file should only contain K.
# Each of the next K lines should contain P+1 integers separated by spaces,
# where P is the number of nodes in the strongly connected component.
# The first integer in each line is P.
# The next P integers in the line are the nodes in the
# strongly connected component.
# Within each strongly connected component, the nodes must be sorted in
# increasing order in terms of their indices.
# The strongly connected components must appear sorted in increasing order
# in terms of the index of their first node.

class Graph(object):
    def __init__(self, g, n, m):
        self.graph = g
        self.nodes = n
        self.edges = m
        self.explored = [0]*(n+1) # indexed by vertex, 0 is never touched

    # Reverses the directed edges of a graph, returning a new graph
    def reverse(self):
        graph = {}
        for i, j_list in self.graph.items():
            for j in j_list:
                if j not in graph:
                    graph[j] = []
                graph[j].append(i)
        return Graph(graph, self.nodes, self.edges)



# Find Strongly Connected Components using Kosaraju's algorithm
# Kosaraju's algorithm works as follows:
#
# Let G be a directed graph and S be an empty stack.
# While S does not contain all vertices:
#   Choose an arbitrary vertex v not in S.
#   Perform a depth-first search starting at v.
#   Each time that depth-first search finishes expanding a vertex u, push u onto S.
# Reverse the directions of all arcs to obtain the transpose graph.
# While S is nonempty:
#   Pop the top vertex v from S.
#   Perform a depth-first search starting at v in the transpose graph.
#   The set of visited vertices will give the strongly connected component containing v;
#   record this and remove all these vertices from the graph G and the stack S.

