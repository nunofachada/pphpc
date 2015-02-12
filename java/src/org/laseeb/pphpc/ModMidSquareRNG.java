/*
 * Copyright (c) 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Instituto Superior Técnico nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *     
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.laseeb.pphpc;

import java.util.Random;
import java.util.concurrent.locks.ReentrantLock;

import org.uncommons.maths.binary.BinaryUtils;
import org.uncommons.maths.random.DefaultSeedGenerator;
import org.uncommons.maths.random.RepeatableRNG;
import org.uncommons.maths.random.SeedException;
import org.uncommons.maths.random.SeedGenerator;

/**
 * A poor random number generator used to test if it the PPHPC simulation 
 * works with poor generators. Very loosely based on the Middle-Square Method
 * from Von Neumann.
 *
 * Note: After testing this RNG with Dieharder it's actually not that bad. 
 * Randu fails much more tests. I'd still wouldn't recommend it though.
 * 
 * @author Nuno Fachada
 */
public class ModMidSquareRNG extends Random implements RepeatableRNG {
	
	/* Generated serial version UID. */
	private static final long serialVersionUID = -8761384561773317806L;

	/* Seed size. */
	private static final int SEED_SIZE_BYTES = 8;
	
	/* RNG seed. */
	private byte[] seed;
	
	/* The RNG state. */
	private long state;

	/* Lock to prevent concurrent modification of the RNG's internal state. */
	private final ReentrantLock lock = new ReentrantLock();

	/**
	 * Creates a new ModMidSquare RNG and seeds it using the default seeding strategy.
	 */
	public ModMidSquareRNG() {
		this(DefaultSeedGenerator.getInstance().generateSeed(SEED_SIZE_BYTES));
	}


	/**
	 * Seed the ModMidSquare RNG using the provided seed generation strategy.
	 * 
	 * @param seedGenerator The seed generation strategy that will provide
	 * the seed value for this RNG.
	 * @throws SeedException If there is a problem generating a seed.
	 */
	public ModMidSquareRNG(SeedGenerator seedGenerator) throws SeedException {
		this(seedGenerator.generateSeed(SEED_SIZE_BYTES));
	}


	/**
	 * Creates an RNG and seeds it with the specified seed data.
	 * 
	 * @param seed The seed data used to initialize the RNG.
	 */
	public ModMidSquareRNG(byte[] seed) {
		
		if (seed == null || seed.length != SEED_SIZE_BYTES) {
			throw new IllegalArgumentException("ModMidSquare RNG requires 32 bits of seed data.");
		}
		this.seed = seed.clone();
		this.state = BinaryUtils.convertBytesToLong(seed, 0);

	}

	/**
	 * @see org.uncommons.maths.random.RepeatableRNG#getSeed()
	 */
	public byte[] getSeed() {
		return seed.clone();
	}

	/**
	 * @see java.util.Random#next(int bits)
	 */
	@Override
	protected int next(int bits) {
		try {
			lock.lock();
			
			this.state = ((this.state + 2) * (this.state + 1)) >> 1;
			return ((int) (this.state & 0xFFFFFFFF)) >>> (32 - bits);

		} finally {
			lock.unlock();
		}
	}
}