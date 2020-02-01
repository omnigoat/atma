using System;
using System.Collections.Generic;
using System.Linq;

namespace OmniEdit.Sourcing
{
	public class ImmutableRedBlackTree<T>
		where T : IComparable<T>
	{
		private class Node
		{
			public enum Coloring
			{
				NegativeBlack = -1,
				Red = 0,
				Black = 1,
				DoubleBlack = 2
			}

			private static int idGen = 0;

			private readonly int id = 0;

			public Coloring Color { get; set; }
			public T Value;

			public Node Parent;
			public Node Left;
			public Node Right;

			public Node(Coloring color = Coloring.Black)
			{
				this.Color = color;
			}

			public Node(Coloring color, Node left, T value, Node right)
			{
				this.id = idGen++;
				this.Color = color;
				this.Left = left;
				this.Right = right;
				this.Value = value;

				if (Left != null)
					Left.Parent = this;

				if (Right != null)
					Right.Parent = this;
			}

			public override string ToString() => string.Format("Node(@{0}) {1}", this.id, Value);

			public T Min => Left != null ? Left.Min : Value;
			public T Max => Right != null ? Right.Max : Value;

			public bool IsRed => Color == Coloring.Red;
			public bool IsBlack => Color == Coloring.Black;
			public bool IsDoubleBlack => Color == Coloring.DoubleBlack;

			private static bool BlackOrDoubleBlack(Coloring color) => color == Coloring.Black || color == Coloring.DoubleBlack;
			private static bool JustDoubleBlack(Coloring color) => color == Coloring.DoubleBlack;
			private static bool JustRed(Coloring color) => color == Coloring.Red;
			private static bool JustBlack(Coloring color) => color == Coloring.Black;
			private static bool JustNegativeBlack(Coloring color) => color == Coloring.NegativeBlack;

			public static bool Match(Coloring color, Node t, ref Node left, ref T value, ref Node right)
				=> Match(x => x == color, t, ref left, ref value, ref right);

			private static bool Match(Func<Coloring, bool> colorPredicate, Node t, ref Node left, ref T value, ref Node right)
			{
				if (t != null && colorPredicate(t.Color))
				{
					left = t.Left;
					value = t.Value;
					right = t.Right;
					return true;
				}

				return false;
			}

			private static bool Match(Func<Coloring, bool> colorPredicate, Coloring color, Node left, T value, Node right, ref Node outLeft, ref T outValue, ref Node outRight)
			{
				if (colorPredicate(color))
				{
					outLeft = left;
					outValue = value;
					outRight = right;
					return true;
				}

				return false;
			}

			public static bool BalanceOp1(Coloring color, Node left, T v, Node right, ref Node a, ref T x, ref Node b, ref T y, ref Node c, ref T z, ref Node d)
			{
				Node tL = null, tLL = null;
				return Match(BlackOrDoubleBlack, color, left, v, right, ref tL, ref z, ref d)
					&& Match(JustRed, tL, ref tLL, ref y, ref c)
					&& Match(JustRed, tLL, ref a, ref x, ref b);
			}

			public static bool BalanceOp2(Coloring color, Node left, T v, Node right, ref Node a, ref T x, ref Node b, ref T y, ref Node c, ref T z, ref Node d)
			{
				Node tL = null, tLR = null;
				return Match(BlackOrDoubleBlack, color, left, v, right, ref tL, ref z, ref d)
					&& Match(JustRed, tL, ref a, ref x, ref tLR)
					&& Match(JustRed, tLR, ref b, ref y, ref c);
			}

			public static bool BalanceOp3(Coloring color, Node left, T v, Node right, ref Node a, ref T x, ref Node b, ref T y, ref Node c, ref T z, ref Node d)
			{
				Node tR = null, tRL = null;
				return Match(BlackOrDoubleBlack, color, left, v, right, ref a, ref x, ref tR)
					&& Match(JustRed, tR, ref tRL, ref z, ref d)
					&& Match(JustRed, tRL, ref b, ref y, ref c);
			}

			public static bool BalanceOp4(Coloring color, Node left, T v, Node right, ref Node a, ref T x, ref Node b, ref T y, ref Node c, ref T z, ref Node d)
			{
				Node tR = null, tRR = null;
				return Match(BlackOrDoubleBlack, color, left, v, right, ref a, ref x, ref tR)
					&& Match(JustRed, tR, ref b, ref y, ref tRR)
					&& Match(JustRed, tRR, ref c, ref z, ref d);
			}

			public static bool BalanceOp_BB1(Coloring color, Node left, T v, Node right, ref Node a, ref T x, ref Node b, ref T y, ref Node c, ref T z, ref Node d)
			{
				Node tR = null, tRL = null;
				return Match(JustDoubleBlack, color, left, v, right, ref a, ref x, ref tR)
					&& Match(JustNegativeBlack, tR, ref tRL, ref z, ref d) && (d?.IsBlack ?? false)
					&& Match(JustBlack, tRL, ref b, ref y, ref c);
			}

			public static bool BalanceOp_BB2(Coloring color, Node left, T v, Node right, ref Node a, ref T x, ref Node b, ref T y, ref Node c, ref T z, ref Node d)
			{
				Node tL = null, tLR = null;
				return Match(JustDoubleBlack, color, left, v, right, ref tL, ref z, ref d)
					&& Match(JustNegativeBlack, tL, ref a, ref x, ref tLR) && (a?.IsBlack ?? false)
					&& Match(JustBlack, tLR, ref b, ref y, ref c);
			}
		}

		private Node root = null;

		// force node to red
		private Node Redden(Node node) => MakeNode(Node.Coloring.Red, node.Left, node.Value, node.Right);

		// make a node more red/more black
		private Node.Coloring MakeRedder(Node.Coloring c) => c - 1;
		private Node.Coloring MakeBlacker(Node.Coloring c) => c + 1;

		private Node MakeRedder(Node node) => node == DoubleEmpty ? Empty : MakeNode(MakeRedder(node.Color), node.Left, node.Value, node.Right);


		private static bool IsDoubleBlack(Node n) => n == DoubleEmpty || n.IsDoubleBlack;


		private static readonly Node Empty = null;
		private static readonly Node DoubleEmpty = new Node(Node.Coloring.DoubleBlack);

		private Node MakeNode(Node.Coloring color, Node left, T value, Node right)
		{
			var n = new Node(color, left, value, right);
			return n;
		}


		// insert
		public bool Insert(T x)
		{
			var r2 = Insertmpl(root, x, out bool success);
			if (r2 != root)
			{
				root = r2;
				root.Color = Node.Coloring.Black;
				Console.WriteLine("insertion yielded new tree:");
				DebugPrint();
			}
			return success;
		}

		private Node Insertmpl(Node n, T x, out bool success)
		{
			if (n == Empty)
			{
				success = true;
				var node = MakeNode(Node.Coloring.Red, null, x, null);
				return node;
			}
			else
			{
				var r = x.CompareTo(n.Value);
				if (r < 0)
				{
					return Balance(n.Color, Insertmpl(n.Left, x, out success), n.Value, n.Right);
				}
				else if (0 < r)
				{
					return Balance(n.Color, n.Left, n.Value, Insertmpl(n.Right, x, out success));
				}
				else
				{
					success = false;
					return n;
				}
			}
		}

		// delete
		public bool Delete(T x)
		{
			root = Delete(root, x);
			root.Color = Node.Coloring.Black;
			return false;
		}

		private Node Delete(Node n, T x)
		{
			if (n == Empty)
				return Empty;

			var r = n.Value.CompareTo(x);
			if (0 < r)
				return Bubble(n.Color, Delete(n.Left, x), n.Value, n.Right);
			else if (r < 0)
				return Bubble(n.Color, n.Left, n.Value, Delete(n.Right, x));
			else
				return Delete2(n);
		}

		private Node Delete2(Node n)
		{
			if (n == Empty)
				return Empty;

			Node a = null, b = null;
			T x = default;

			if (n.IsRed && n.Left == Empty && n.Right == Empty)
				return Empty;
			else if (n.IsBlack && n.Left == Empty && n.Right == Empty)
				return DoubleEmpty;
			else if (n.IsBlack && n.Left == Empty && Node.Match(Node.Coloring.Red, n.Right, ref a, ref x, ref b))
				return MakeNode(Node.Coloring.Black, a, x, b);
			else if (n.IsBlack && n.Right == Empty && Node.Match(Node.Coloring.Red, n.Left, ref a, ref x, ref b))
				return MakeNode(Node.Coloring.Black, a, x, b);
			else
				return Bubble(n.Color, DeleteMax(n.Left), n.Left.Max, n.Right);
		}

		private Node DeleteMax(Node n) => (n.Right == Empty) ? Delete2(n) : Bubble(n.Color, n.Left, n.Value, DeleteMax(n.Right));


		private Node Balance(Node.Coloring color, Node left, T v, Node right)
		{
			Node a = null, b = null, c = null, d = null;
			T x = default, y = default, z = default;

			if (Node.BalanceOp1(color, left, v, right, ref a, ref x, ref b, ref y, ref c, ref z, ref d) ||
				Node.BalanceOp2(color, left, v, right, ref a, ref x, ref b, ref y, ref c, ref z, ref d) ||
				Node.BalanceOp3(color, left, v, right, ref a, ref x, ref b, ref y, ref c, ref z, ref d) ||
				Node.BalanceOp4(color, left, v, right, ref a, ref x, ref b, ref y, ref c, ref z, ref d))
			{
				var l = MakeNode(Node.Coloring.Black, a, x, b);
				var r = MakeNode(Node.Coloring.Black, c, z, d);
				return MakeNode(Node.Coloring.Red, l, y, r);
			}
			else if (Node.BalanceOp_BB1(color, left, v, right, ref a, ref x, ref b, ref y, ref c, ref z, ref d))
			{
				var l = MakeNode(Node.Coloring.Black, a, x, b);
				var r = Balance(Node.Coloring.Black, c, z, Redden(d));
				return MakeNode(Node.Coloring.Black, l, y, r);
			}
			else if (Node.BalanceOp_BB2(color, left, v, right, ref a, ref x, ref b, ref y, ref c, ref z, ref d))
			{
				var l = Balance(Node.Coloring.Black, Redden(a), x, b);
				var r = MakeNode(Node.Coloring.Black, c, z, d);
				return MakeNode(Node.Coloring.Black, l, y, r);
			}
			else
			{
				return MakeNode(color, left, v, right);
			}
		}


		private Node Bubble(Node.Coloring color, Node left, T x, Node right)
		{
			if (IsDoubleBlack(left) || IsDoubleBlack(right))
				return Balance(MakeBlacker(color), MakeRedder(left), x, MakeRedder(right));
			else
				return Balance(color, left, x, right);
		}

		public void DebugPrint()
		{
			List<Node> nodes = new List<Node> { root };
			List<Node> nextNodes = new List<Node>();

			while (nodes.Any())
			{
				foreach (var n in nodes)
				{
					if (n != null)
					{
						nextNodes.Add(n.Left);
						nextNodes.Add(n.Right);

						Console.Write("{0}: {1} ", n.IsRed ? 'R' : 'B', n.Value);
					}
					else
					{
						Console.Write("<null> ");
					}
				}

				Console.WriteLine();

				nodes = nextNodes;
				nextNodes = new List<Node>();
			}
		}

	}

}
