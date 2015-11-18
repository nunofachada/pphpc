/*
 * Copyright (c) 2014, 2015, Nuno Fachada
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
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
 * Implementation of the RANDU random number generator. It is a poor quality
 * generator used to test if it the PPHPC simulation works with poor generators.
 *
 * @author Nuno Fachada
 */
public class RanduRNG extends Random implements RepeatableRNG {
	
	/* Generated serial version UID. */
	private static final long serialVersionUID = -8761384561773317806L;

	/* Seed size for RANDU. */
	private static final int SEED_SIZE_BYTES = 4;
	
	/* The poor constant for RANDU. */
	private final int a = 65539;

	/* RNG seed. */
	private byte[] seed;
	
	/* The RNG state. */
	private int state;

	/* Lock to prevent concurrent modification of the RNG's internal state. */
	private final ReentrantLock lock = new ReentrantLock();

	/**
	 * Creates a new RANDU RNG and seeds it using the default seeding strategy.
	 */
	public RanduRNG() {
		this(DefaultSeedGenerator.getInstance().generateSeed(SEED_SIZE_BYTES));
	}


	/**
	 * Seed the RANDU RNG using the provided seed generation strategy.
	 * 
	 * @param seedGenerator The seed generation strategy that will provide
	 * the seed value for this RNG.
	 * @throws SeedException If there is a problem generating a seed.
	 */
	public RanduRNG(SeedGenerator seedGenerator) throws SeedException {
		this(seedGenerator.generateSeed(SEED_SIZE_BYTES));
	}


	/**
	 * Creates an RNG and seeds it with the specified seed data.
	 * 
	 * @param seed The seed data used to initialize the RNG.
	 */
	public RanduRNG(byte[] seed) {
		
		if (seed == null || seed.length != SEED_SIZE_BYTES) {
			throw new IllegalArgumentException("RANDU RNG requires 32 bits of seed data.");
		}
		this.seed = seed.clone();
		int[] state = BinaryUtils.convertBytesToInts(seed);
		this.state = state[0];

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
			this.state = this.a * this.state;
			return this.state >>> (32 - bits);
		} finally {
			lock.unlock();
		}
	}
}