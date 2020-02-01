using System;
using System.Collections.Generic;
using System.Text;

namespace Atma.ImmutableRopeTree
{
	internal class Node<T>
		where T : IComparable<T>
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

		public Node<T> Parent;
		public Node<T> Left;
		public Node<T> Right;

		public Node(Coloring color = Coloring.Black)
		{
			this.Color = color;
		}

		public Node(Coloring color, Node<T> left, T value, Node<T> right)
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

		public static bool Match(Coloring color, Node<T> t, ref Node<T> left, ref T value, ref Node<T> right)
			=> Match(x => x == color, t, ref left, ref value, ref right);

		private static bool Match(Func<Coloring, bool> colorPredicate, Node<T> t, ref Node<T> left, ref T value, ref Node<T> right)
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

		private static bool Match(Func<Coloring, bool> colorPredicate, Coloring color, Node<T> left, T value, Node<T> right, ref Node<T> outLeft, ref T outValue, ref Node<T> outRight)
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

		public static bool BalanceOp1(Coloring color, Node<T> left, T v, Node<T> right, ref Node<T> a, ref T x, ref Node<T> b, ref T y, ref Node<T> c, ref T z, ref Node<T> d)
		{
			Node<T> tL = null, tLL = null;
			return Match(BlackOrDoubleBlack, color, left, v, right, ref tL, ref z, ref d)
				&& Match(JustRed, tL, ref tLL, ref y, ref c)
				&& Match(JustRed, tLL, ref a, ref x, ref b);
		}

		public static bool BalanceOp2(Coloring color, Node<T> left, T v, Node<T> right, ref Node<T> a, ref T x, ref Node<T> b, ref T y, ref Node<T> c, ref T z, ref Node<T> d)
		{
			Node<T> tL = null, tLR = null;
			return Match(BlackOrDoubleBlack, color, left, v, right, ref tL, ref z, ref d)
				&& Match(JustRed, tL, ref a, ref x, ref tLR)
				&& Match(JustRed, tLR, ref b, ref y, ref c);
		}

		public static bool BalanceOp3(Coloring color, Node<T> left, T v, Node<T> right, ref Node<T> a, ref T x, ref Node<T> b, ref T y, ref Node<T> c, ref T z, ref Node<T> d)
		{
			Node<T> tR = null, tRL = null;
			return Match(BlackOrDoubleBlack, color, left, v, right, ref a, ref x, ref tR)
				&& Match(JustRed, tR, ref tRL, ref z, ref d)
				&& Match(JustRed, tRL, ref b, ref y, ref c);
		}

		public static bool BalanceOp4(Coloring color, Node<T> left, T v, Node<T> right, ref Node<T> a, ref T x, ref Node<T> b, ref T y, ref Node<T> c, ref T z, ref Node<T> d)
		{
			Node<T> tR = null, tRR = null;
			return Match(BlackOrDoubleBlack, color, left, v, right, ref a, ref x, ref tR)
				&& Match(JustRed, tR, ref b, ref y, ref tRR)
				&& Match(JustRed, tRR, ref c, ref z, ref d);
		}

		public static bool BalanceOp_BB1(Coloring color, Node<T> left, T v, Node<T> right, ref Node<T> a, ref T x, ref Node<T> b, ref T y, ref Node<T> c, ref T z, ref Node<T> d)
		{
			Node<T> tR = null, tRL = null;
			return Match(JustDoubleBlack, color, left, v, right, ref a, ref x, ref tR)
				&& Match(JustNegativeBlack, tR, ref tRL, ref z, ref d) && (d?.IsBlack ?? false)
				&& Match(JustBlack, tRL, ref b, ref y, ref c);
		}

		public static bool BalanceOp_BB2(Coloring color, Node<T> left, T v, Node<T> right, ref Node<T> a, ref T x, ref Node<T> b, ref T y, ref Node<T> c, ref T z, ref Node<T> d)
		{
			Node<T> tL = null, tLR = null;
			return Match(JustDoubleBlack, color, left, v, right, ref tL, ref z, ref d)
				&& Match(JustNegativeBlack, tL, ref a, ref x, ref tLR) && (a?.IsBlack ?? false)
				&& Match(JustBlack, tLR, ref b, ref y, ref c);
		}
	}
}
