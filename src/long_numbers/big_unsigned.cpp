#include "big_unsigned.h"

namespace lipaboy_lib {

	namespace long_numbers_space {

		// Memory management definitions have moved to the bottom of NumberlikeArray.hh.

		// The templates used by these constructors and converters are at the bottom of
		// BigUnsigned.hh.

		BigUnsigned::BigUnsigned(unsigned long  x) { initFromPrimitive(x); }
		BigUnsigned::BigUnsigned(unsigned int   x) { initFromPrimitive(x); }
		BigUnsigned::BigUnsigned(unsigned short x) { initFromPrimitive(x); }
		BigUnsigned::BigUnsigned(long  x) { initFromSignedPrimitive(x); }
		BigUnsigned::BigUnsigned(int   x) { initFromSignedPrimitive(x); }
		BigUnsigned::BigUnsigned(short x) { initFromSignedPrimitive(x); }

		unsigned long  BigUnsigned::toUnsignedLong() const { return convertToPrimitive      <unsigned long >(); }
		unsigned int   BigUnsigned::toUnsignedInt() const { return convertToPrimitive      <unsigned int  >(); }
		unsigned short BigUnsigned::toUnsignedShort() const { return convertToPrimitive      <unsigned short>(); }
		long           BigUnsigned::toLong() const { return convertToSignedPrimitive<         long >(); }
		int            BigUnsigned::toInt() const { return convertToSignedPrimitive<         int  >(); }
		short          BigUnsigned::toShort() const { return convertToSignedPrimitive<         short>(); }

		// BIT/BLOCK ACCESSORS

		void BigUnsigned::setBlock(IndexType i, BlockType newBlock) {
			if (newBlock == 0) {
				if (i < getLength()) {
					blocks[i] = 0;
					zapLeadingZeros();
				}
			}
			else {
				if (i >= getLength()) {
					// The nonzero block extends the number.
					blocks.resize(i + 1, 0);
					// Zero any added blocks that we aren't setting.
					/*for (IndexType j = old size; j < i; j++)
						blocks[j] = 0;*/
				}
				blocks[i] = newBlock;
			}
		}

		/* Evidently the compiler wants BigUnsigned:: on the return type because, at
		 * that point, it hasn't yet parsed the BigUnsigned:: on the name to get the
		 * proper scope. */
		BigUnsigned::IndexType BigUnsigned::bitLength() const {
			if (isZero())
				return 0;
			else {
				BlockType leftmostBlock = getBlock(getLength() - 1);
				IndexType leftmostBlockLen = 0;
				while (leftmostBlock != 0) {
					leftmostBlock >>= 1;
					leftmostBlockLen++;
				}
				return leftmostBlockLen + (getLength() - 1) * BITS_PER_BLOCK;
			}
		}

		void BigUnsigned::setBit(IndexType bi, bool newBit) {
			IndexType blockI = bi / BITS_PER_BLOCK;
			BlockType block = getBlock(blockI), mask = BlockType(1) << (bi % BITS_PER_BLOCK);
			block = newBit ? (block | mask) : (block & ~mask);
			setBlock(blockI, block);
		}

		// COMPARISON
		BigUnsigned::CmpRes BigUnsigned::compareTo(const BigUnsigned &x) const {
			// A bigger getLength()gth implies a bigger number.
			if (getLength() < x.getLength())
				return less;
			else if (getLength() > x.getLength())
				return greater;
			else {
				// Compare blocks one by one from left to right.
				IndexType i = getLength();
				while (i > 0) {
					i--;
					if (blocks[i] == x.blocks[i])
						continue;
					else if (blocks[i] > x.blocks[i])
						return greater;
					else
						return less;
				}
				// If no blocks differed, the numbers are equal.
				return equal;
			}
		}

		// COPY-LESS OPERATIONS

		/*
		 * On most calls to copy-less operations, it's safe to read the inputs little by
		 * little and write the outputs little by little.  However, if one of the
		 * inputs is coming from the same variable into which the output is to be
		 * stored (an "aliased" call), we risk overwriting the input before we read it.
		 * In this case, we first compute the result into a temporary BigUnsigned
		 * variable and then copy it into the requested output variable *this.
		 * Each put-here operation uses the DTRT_ALIASED macro (Do The Right Thing on
		 * aliased calls) to generate code for this check.
		 *
		 * I adopted this approach on 2007.02.13 (see Assignment Operators in
		 * BigUnsigned.hh).  Before then, put-here operations rejected aliased calls
		 * with an exception.  I think doing the right thing is better.
		 *
		 * Some of the put-here operations can probably handle aliased calls safely
		 * without the extra copy because (for example) they process blocks strictly
		 * right-to-left.  At some point I might determine which ones don't need the
		 * copy, but my reasoning would need to be verified very carefully.  For now
		 * I'll leave in the copy.
		 */

		// TODO: Replace this macros on smth else
#define DTRT_ALIASED(cond, op) \
	if (cond) { \
		BigUnsigned tmpThis; \
		tmpThis.op; \
		*this = tmpThis; \
		return; \
	}

		void BigUnsigned::add(const BigUnsigned &a, const BigUnsigned &b) {
			DTRT_ALIASED(this == &a || this == &b, add(a, b));
			// If one argument is zero, copy the other.
			if (a.isZero()) {
				operator =(b);
				return;
			}
			else if (b.isZero()) {
				operator =(a);
				return;
			}
			// Some variables...
			// Carries in and out of an addition stage
			bool carryIn, carryOut;
			BlockType temp;
			IndexType i;
			// a2 points to the longer input, b2 points to the shorter
			const BigUnsigned *a2, *b2;
			if (a.getLength() >= b.getLength()) {
				a2 = &a;
				b2 = &b;
			}
			else {
				a2 = &b;
				b2 = &a;
			}
			// Set prelimiary getLength()gth and make room in this BigUnsigned
			blocks.resize(a2->getLength() + 1);
			// For each block index that is present in both inputs...
			for (i = 0, carryIn = false; i < b2->getLength(); i++) {
				// Add input blocks
				temp = a2->blocks[i] + b2->blocks[i];
				// If a rollover occurred, the result is less than either input.
				// This test is used many times in the BigUnsigned code.
				carryOut = (temp < a2->blocks[i]);
				// If a carry was input, handle it
				if (carryIn) {
					temp++;
					carryOut |= (temp == 0);
				}
				blocks[i] = temp; // Save the addition result
				carryIn = carryOut; // Pass the carry along
			}
			// If there is a carry left over, increase blocks until
			// one does not roll over.
			for (; i < a2->getLength() && carryIn; i++) {
				temp = a2->blocks[i] + 1;
				carryIn = (temp == 0);
				blocks[i] = temp;
			}
			// If the carry was resolved but the larger number
			// still has blocks, copy them over.
			for (; i < a2 -> getLength(); i++)
				blocks[i] = a2->blocks[i];
			// Set the extra block if there's still a carry, decrease getLength()gth otherwise
			if (carryIn)
				blocks[i] = 1;
			else
				blocks.resize(getLength() - 1);
		}

		void BigUnsigned::subtract(const BigUnsigned &a, const BigUnsigned &b) {
			DTRT_ALIASED(this == &a || this == &b, subtract(a, b));
			if (b.isZero()) {
				// If b is zero, copy a.
				operator =(a);
				return;
			}
			else if (a.getLength() < b.getLength())
				// If a is shorter than b, the result is negative.
				throw "BigUnsigned::subtract: "
				"Negative result in unsigned calculation";
			// Some variables...
			bool borrowIn, borrowOut;
			BlockType temp;
			IndexType i;
			// Set preliminary getLength()gth and make room
			blocks.resize(a.getLength());
			// For each block index that is present in both inputs...
			for (i = 0, borrowIn = false; i < b.getLength(); i++) {
				temp = a.blocks[i] - b.blocks[i];
				// If a reverse rollover occurred,
				// the result is greater than the block from a.
				borrowOut = (temp > a.blocks[i]);
				// Handle an incoming borrow
				if (borrowIn) {
					borrowOut |= (temp == 0);
					temp--;
				}
				blocks[i] = temp; // Save the subtraction result
				borrowIn = borrowOut; // Pass the borrow along
			}
			// If there is a borrow left over, decrease blocks until
			// one does not reverse rollover.
			for (; i < a.getLength() && borrowIn; i++) {
				borrowIn = (a.blocks[i] == 0);
				blocks[i] = a.blocks[i] - 1;
			}
			/* If there's still a borrow, the result is negative.
			 * Throw an exception, but zero out this object so as to leave it in a
			 * predictable state. */
			if (borrowIn) {
				setToZero();
				throw "BigUnsigned::subtract: Negative result in unsigned calculation";
			}
			else
				// Copy over the rest of the blocks
				for (; i < a.getLength(); i++)
					blocks[i] = a.blocks[i];
			// Zap leading zeros
			zapLeadingZeros();
		}

		/*
		 * About the multiplication and division algorithms:
		 *
		 * I searched unsucessfully for fast C++ built-in operations like the `b_0'
		 * and `c_0' Knuth describes in Section 4.3.1 of ``The Art of Computer
		 * Programming'' (replace `place' by `Blk'):
		 *
		 *    ``b_0[:] multiplication of a one-place integer by another one-place
		 *      integer, giving a two-place answer;
		 *
		 *    ``c_0[:] division of a two-place integer by a one-place integer,
		 *      provided that the quotient is a one-place integer, and yielding
		 *      also a one-place remainder.''
		 *
		 * I also missed his note that ``[b]y adjusting the word size, if
		 * necessary, nearly all computers will have these three operations
		 * available'', so I gave up on trying to use algorithms similar to his.
		 * A future version of the library might include such algorithms; I
		 * would welcome contributions from others for this.
		 *
		 * I eventually decided to use bit-shifting algorithms.  To multiply `a'
		 * and `b', we zero out the result.  Then, for each `1' bit in `a', we
		 * shift `b' left the appropriate amount and add it to the result.
		 * Similarly, to divide `a' by `b', we shift `b' left varying amounts,
		 * repeatedly trying to subtract it from `a'.  When we succeed, we note
		 * the fact by setting a bit in the quotient.  While these algorithms
		 * have the same O(n^2) time complexity as Knuth's, the ``constant factor''
		 * is likely to be larger.
		 *
		 * Because I used these algorithms, which require single-block addition
		 * and subtraction rather than single-block multiplication and division,
		 * the innermost loops of all four routines are very similar.  Study one
		 * of them and all will become clear.
		 */

		 /*
		  * This is a little inline function used by both the multiplication
		  * routine and the division routine.
		  *
		  * `getShiftedBlock' returns the `x'th block of `num << y'.
		  * `y' may be anything from 0 to N - 1, and `x' may be anything from
		  * 0 to `num.getLength()'.
		  *
		  * Two things contribute to this block:
		  *
		  * (1) The `N - y' low bits of `num.blk[x]', shifted `y' bits left.
		  *
		  * (2) The `y' high bits of `num.blk[x-1]', shifted `N - y' bits right.
		  *
		  * But we must be careful if `x == 0' or `x == num.getLength()', in
		  * which case we should use 0 instead of (2) or (1), respectively.
		  *
		  * If `y == 0', then (2) contributes 0, as it should.  However,
		  * in some computer environments, for a reason I cannot understand,
		  * `a >> b' means `a >> (b % N)'.  This means `num.blk[x-1] >> (N - y)'
		  * will return `num.blk[x-1]' instead of the desired 0 when `y == 0';
		  * the test `y == 0' handles this case specially.
		  */
		inline BigUnsigned::BlockType getShiftedBlock(const BigUnsigned &num,
			BigUnsigned::IndexType x, unsigned int y) {
			BigUnsigned::BlockType part1 = (x == 0 || y == 0) ? 0 
				: (num.blocks[x - 1] >> (BigUnsigned::BITS_PER_BLOCK - y));
			BigUnsigned::BlockType part2 = (x == num.getLength()) ? 0 : (num.blocks[x] << y);
			return part1 | part2;
		}

		void BigUnsigned::multiply(const BigUnsigned &a, const BigUnsigned &b) {
			DTRT_ALIASED(this == &a || this == &b, multiply(a, b));
			// If either a or b is zero, set to zero.
			if (a.isZero() || b.isZero()) {
				setToZero();
				return;
			}
			/*
			 * Overall method:
			 *
			 * Set this = 0.
			 * For each 1-bit of `a' (say the `i2'th bit of block `i'):
			 *    Add `b << (i blocks and i2 bits)' to *this.
			 */
			 // Variables for the calculation
			IndexType i, j, k;
			unsigned int i2;
			BlockType temp;
			bool carryIn, carryOut;
			// Set preliminary getLength()gth and make room
			blocks.resize(a.getLength() + b.getLength());
			// Zero out this object
			for (i = 0; i < getLength(); i++)
				blocks[i] = 0;
			// For each block of the first number...
			for (i = 0; i < a.getLength(); i++) {
				// For each 1-bit of that block...
				for (i2 = 0; i2 < BITS_PER_BLOCK; i2++) {
					if ((a.blocks[i] & (BlockType(1) << i2)) == 0)
						continue;
					/*
					 * Add b to this, shifted left i blocks and i2 bits.
					 * j is the index in b, and k = i + j is the index in this.
					 *
					 * `getShiftedBlock', a short inline function defined above,
					 * is now used for the bit handling.  It replaces the more
					 * complex `bHigh' code, in which each run of the loop dealt
					 * immediately with the low bits and saved the high bits to
					 * be picked up next time.  The last run of the loop used to
					 * leave leftover high bits, which were handled separately.
					 * Instead, this loop runs an additional time with j == b.getLength().
					 * These changes were made on 2005.01.11.
					 */
					for (j = 0, k = i, carryIn = false; j <= b.getLength(); j++, k++) {
						/*
						 * The body of this loop is very similar to the body of the first loop
						 * in `add', except that this loop does a `+=' instead of a `+'.
						 */
						temp = blocks[k] + getShiftedBlock(b, j, i2);
						carryOut = (temp < blocks[k]);
						if (carryIn) {
							temp++;
							carryOut |= (temp == 0);
						}
						blocks[k] = temp;
						carryIn = carryOut;
					}
					// No more extra iteration to deal with `bHigh'.
					// Roll-over a carry as necessary.
					for (; carryIn; k++) {
						blocks[k]++;
						carryIn = (blocks[k] == 0);
					}
				}
			}
			// Zap possible leading zero
			if (blocks[getLength() - 1] == 0)
				blocks.resize(getLength() - 1);
		}

		/*
		 * DIVISION WITH REMAINDER
		 * This monstrous function mods *this by the given divisor b while storing the
		 * quotient in the given object q; at the end, *this contains the remainder.
		 * The seemingly bizarre pattern of inputs and outputs was chosen so that the
		 * function copies as little as possible (since it is implemented by repeated
		 * subtraction of multiples of b from *this).
		 *
		 * "modWithQuotient" might be a better name for this function, but I would
		 * rather not change the name now.
		 */
		void BigUnsigned::divideWithRemainder(const BigUnsigned &b, BigUnsigned &q) {
			/* Defending against aliased calls is more complex than usual because we
			 * are writing to both *this and q.
			 *
			 * It would be silly to try to write quotient and remainder to the
			 * same variable.  Rule that out right away. */
			if (this == &q)
				throw "BigUnsigned::divideWithRemainder: Cannot write quotient and remainder into the same variable";
			/* Now *this and q are separate, so the only concern is that b might be
			 * aliased to one of them.  If so, use a temporary copy of b. */
			if (this == &b || &q == &b) {
				BigUnsigned tmpB(b);
				divideWithRemainder(tmpB, q);
				return;
			}

			/*
			 * Knuth's definition of mod (which this function uses) is somewhat
			 * different from the C++ definition of % in case of division by 0.
			 *
			 * We let a / 0 == 0 (it doesn't matter much) and a % 0 == a, no
			 * exceptions thrown.  This allows us to preserve both Knuth's demand
			 * that a mod 0 == a and the useful property that
			 * (a / b) * b + (a % b) == a.
			 */
			if (b.isZero()) {
				q.setToZero();
				return;
			}

			/*
			 * If *this.getLength() < b.getLength(), then *this < b, and we can be sure that b doesn't go into
			 * *this at all.  The quotient is 0 and *this is already the remainder (so leave it alone).
			 */
			if (getLength() < b.getLength()) {
				q.setToZero();
				return;
			}

			// At this point we know (*this).getLength() >= b.getLength() > 0.  (Whew!)

			/*
			 * Overall method:
			 *
			 * For each appropriate i and i2, decreasing:
			 *    Subtract (b << (i blocks and i2 bits)) from *this, storing the
			 *      result in subtractBuf.
			 *    If the subtraction succeeds with a nonnegative result:
			 *        Turn on bit i2 of block i of the quotient q.
			 *        Copy subtractBuf back into *this.
			 *    Otherwise bit i2 of block i remains off, and *this is unchanged.
			 *
			 * Eventually q will contain the entire quotient, and *this will
			 * be left with the remainder.
			 *
			 * subtractBuf[x] corresponds to blk[x], not blk[x+i], since 2005.01.11.
			 * But on a single iteration, we don't touch the i lowest blocks of blk
			 * (and don't use those of subtractBuf) because these blocks are
			 * unaffected by the subtraction: we are subtracting
			 * (b << (i blocks and i2 bits)), which ends in at least `i' zero
			 * blocks. */
			 // Variables for the calculation
			IndexType i, j, k;
			unsigned int i2;
			BlockType temp;
			bool borrowIn, borrowOut;

			/*
			 * Make sure we have an extra zero block just past the value.
			 *
			 * When we attempt a subtraction, we might shift `b' so
			 * its first block begins a few bits left of the dividend,
			 * and then we'll try to compare these extra bits with
			 * a nonexistent block to the left of the dividend.  The
			 * extra zero block ensures sensible behavior; we need
			 * an extra block in `subtractBuf' for exactly the same reason.
			 */
			IndexType origLen = getLength(); // Save real getLength()gth.
			/* To avoid an out-of-bounds access in case of reallocation, allocate
			 * first and then increment the logical getLength()gth. */
			blocks.resize(getLength() + 1);
			blocks[origLen] = 0; // Zero the added block.

			// subtractBuf holds part of the result of a subtraction; see above.
			BlockType *subtractBuf = new BlockType[getLength()];

			// Set preliminary getLength()gth for quotient and make room
			q.blocks.resize(origLen - b.getLength() + 1);
			// Zero out the quotient
			for (i = 0; i < q.getLength(); i++)
				q.blocks[i] = 0;

			// For each possible left-shift of b in blocks...
			i = q.getLength();
			while (i > 0) {
				i--;
				// For each possible left-shift of b in bits...
				// (Remember, N is the number of bits in a Blk.)
				q.blocks[i] = 0;
				i2 = BITS_PER_BLOCK;
				while (i2 > 0) {
					i2--;
					/*
					 * Subtract b, shifted left i blocks and i2 bits, from *this,
					 * and store the answer in subtractBuf.  In the for loop, `k == i + j'.
					 *
					 * Compare this to the middle section of `multiply'.  They
					 * are in many ways analogous.  See especially the discussion
					 * of `getShiftedBlock'.
					 */
					for (j = 0, k = i, borrowIn = false; j <= b.getLength(); j++, k++) {
						temp = blocks[k] - getShiftedBlock(b, j, i2);
						borrowOut = (temp > blocks[k]);
						if (borrowIn) {
							borrowOut |= (temp == 0);
							temp--;
						}
						// Since 2005.01.11, indices of `subtractBuf' directly match those of `blk', so use `k'.
						subtractBuf[k] = temp;
						borrowIn = borrowOut;
					}
					// No more extra iteration to deal with `bHigh'.
					// Roll-over a borrow as necessary.
					for (; k < origLen && borrowIn; k++) {
						borrowIn = (blocks[k] == 0);
						subtractBuf[k] = blocks[k] - 1;
					}
					/*
					 * If the subtraction was performed successfully (!borrowIn),
					 * set bit i2 in block i of the quotient.
					 *
					 * Then, copy the portion of subtractBuf filled by the subtraction
					 * back to *this.  This portion starts with block i and ends--
					 * where?  Not necessarily at block `i + b.getLength()'!  Well, we
					 * increased k every time we saved a block into subtractBuf, so
					 * the region of subtractBuf we copy is just [i, k).
					 */
					if (!borrowIn) {
						q.blocks[i] |= (BlockType(1) << i2);
						while (k > i) {
							k--;
							blocks[k] = subtractBuf[k];
						}
					}
				}
			}
			// Zap possible leading zero in quotient
			if (q.blocks[q.getLength() - 1] == 0)
				q.blocks.resize(q.getLength() - 1);
			// Zap any/all leading zeros in remainder
			zapLeadingZeros();
			// Deallocate subtractBuf.
			// (Thanks to Brad Spencer for noticing my accidental omission of this!)
			delete[] subtractBuf;
		}

		/* BITWISE OPERATORS
		 * These are straightforward blockwise operations except that they differ in
		 * the output getLength()gth and the necessity of zapLeadingZeros. */

		void BigUnsigned::bitAnd(const BigUnsigned &a, const BigUnsigned &b) {
			DTRT_ALIASED(this == &a || this == &b, bitAnd(a, b));
			// The bitwise & can't be longer than either operand.
			blocks.resize((a.getLength() >= b.getLength()) ? b.getLength() : a.getLength());
			IndexType i;
			for (i = 0; i < getLength(); i++)
				blocks[i] = a.blocks[i] & b.blocks[i];
			zapLeadingZeros();
		}

		void BigUnsigned::bitOr(const BigUnsigned &a, const BigUnsigned &b) {
			DTRT_ALIASED(this == &a || this == &b, bitOr(a, b));
			IndexType i;
			const BigUnsigned *a2, *b2;
			if (a.getLength() >= b.getLength()) {
				a2 = &a;
				b2 = &b;
			}
			else {
				a2 = &b;
				b2 = &a;
			}
			blocks.resize(a2->getLength());
			for (i = 0; i < b2->getLength(); i++)
				blocks[i] = a2->blocks[i] | b2->blocks[i];
			for (; i < a2->getLength(); i++)
				blocks[i] = a2->blocks[i];
			blocks.resize(a2->getLength());
			// Doesn't need zapLeadingZeros.
		}

		void BigUnsigned::bitXor(const BigUnsigned &a, const BigUnsigned &b) {
			DTRT_ALIASED(this == &a || this == &b, bitXor(a, b));
			IndexType i;
			const BigUnsigned *a2, *b2;
			if (a.getLength() >= b.getLength()) {
				a2 = &a;
				b2 = &b;
			}
			else {
				a2 = &b;
				b2 = &a;
			}
			blocks.resize(a2->getLength());
			for (i = 0; i < b2->getLength(); i++)
				blocks[i] = a2->blocks[i] ^ b2->blocks[i];
			for (; i < a2->getLength(); i++)
				blocks[i] = a2->blocks[i];
			blocks.resize(a2->getLength());
			zapLeadingZeros();
		}

		void BigUnsigned::bitShiftLeft(const BigUnsigned &a, int b) {
			DTRT_ALIASED(this == &a, bitShiftLeft(a, b));
			if (b < 0) {
				if (b << 1 == 0)
					throw "BigUnsigned::bitShiftLeft: "
					"Pathological shift amount not implemented";
				else {
					bitShiftRight(a, -b);
					return;
				}
			}
			IndexType shiftBlocks = b / BITS_PER_BLOCK;
			unsigned int shiftBits = b % BITS_PER_BLOCK;
			// + 1: room for high bits nudged left into another block
			blocks.resize(a.getLength() + shiftBlocks + 1);
			IndexType i, j;
			for (i = 0; i < shiftBlocks; i++)
				blocks[i] = 0;
			for (j = 0, i = shiftBlocks; j <= a.getLength(); j++, i++)
				blocks[i] = getShiftedBlock(a, j, shiftBits);
			// Zap possible leading zero
			if (blocks[getLength() - 1] == 0)
				blocks.resize(getLength() - 1);
		}

		void BigUnsigned::bitShiftRight(const BigUnsigned &a, int b) {
			DTRT_ALIASED(this == &a, bitShiftRight(a, b));
			if (b < 0) {
				if (b << 1 == 0)
					throw "BigUnsigned::bitShiftRight: "
					"Pathological shift amount not implemented";
				else {
					bitShiftLeft(a, -b);
					return;
				}
			}
			// This calculation is wacky, but expressing the shift as a left bit shift
			// within each block lets us use getShiftedBlock.
			IndexType rightShiftBlocks = (b + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
			unsigned int leftShiftBits = BITS_PER_BLOCK * rightShiftBlocks - b;
			// Now (N * rightShiftBlocks - leftShiftBits) == b
			// and 0 <= leftShiftBits < N.
			if (rightShiftBlocks >= a.getLength() + 1) {
				// All of a is guaranteed to be shifted off, even considering the left
				// bit shift.
				setToZero();
				return;
			}
			// Now we're allocating a positive amount.
			// + 1: room for high bits nudged left into another block
			blocks.resize(a.getLength() + 1 - rightShiftBlocks);
			IndexType i, j;
			for (j = rightShiftBlocks, i = 0; j <= a.getLength(); j++, i++)
				blocks[i] = getShiftedBlock(a, j, leftShiftBits);
			// Zap possible leading zero
			if (blocks[getLength() - 1] == 0)
				blocks.resize(getLength() - 1);
		}

		// INCREMENT/DECREMENT OPERATORS

		// Prefix increment
		void BigUnsigned::operator ++() {
			IndexType i;
			bool carry = true;
			for (i = 0; i < getLength() && carry; i++) {
				blocks[i]++;
				carry = (blocks[i] == 0);
			}
			if (carry) {
				// Allocate and then increase getLength()gth, as in divideWithRemainder
				blocks.resize(getLength() + 1);
				blocks[i] = 1;
			}
		}

		// Postfix increment: same as prefix
		void BigUnsigned::operator ++(int) {
			operator ++();
		}

		// Prefix decrement
		void BigUnsigned::operator --() {
			if (isZero())
				throw "BigUnsigned::operator --(): Cannot decrement an unsigned zero";
			IndexType i;
			bool borrow = true;
			for (i = 0; borrow; i++) {
				borrow = (blocks[i] == 0);
				blocks[i]--;
			}
			// Zap possible leading zero (there can only be one)
			if (blocks[getLength() - 1] == 0)
				blocks.resize(getLength() - 1);
		}

		// Postfix decrement: same as prefix
		void BigUnsigned::operator --(int) {
			operator --();
		}

	}

}
