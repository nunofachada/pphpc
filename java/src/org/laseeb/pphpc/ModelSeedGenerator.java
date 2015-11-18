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

import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

import org.uncommons.maths.random.SeedException;
import org.uncommons.maths.random.SeedGenerator;

/**
 * A deterministic seed generator for the Uncommons Maths random number 
 * generators. Can produce seeds of any size using a base BigInteger seed.
 * Also supports generating different seeds for different threads based on
 * the same base seed, as to allow for deterministic parallel streams of
 * pseudo-random numbers.
 * 
 * @author Nuno Fachada
 */
public class ModelSeedGenerator implements SeedGenerator {
	
	/* Different threads should pass different IDs, so that each
	 * gets a "different" stream from the RNG. */
	private long wId;
	
	/* Base seed for this seed generator. */
	private BigInteger seed;
	
	/**
	 * Create a new seed generator.
	 * 
	 * @param wId A thread ID or similar, used for producing different
	 * seeds for different threads, based on the same {@link #seed}. If only
	 * one thread is used, pass zero.
	 * @param seed Base seed for this seed generator.
	 */
	public ModelSeedGenerator(int wId, BigInteger seed) {
		this.wId = wId;
		this.seed = seed;
	}

	/**
	 * Generate a seed of a given length in bytes.
	 * 
	 * @param length Length in bytes of seed to generate.
	 * @throws SeedException if something bad happens.
	 */
	@Override
	public byte[] generateSeed(int length) throws SeedException {
		
		/* Final seed to be generated. */
		BigInteger finalSeed = this.seed;
		
		/* If the thread modifier is not zero, create a scrambled modifierXor, 
		 * unique for each thread. */
		if (wId != 0) {

			/* This modifier will be XOR'ed with the final seed in order to
			 * produce different seeds for different threads. */
			BigInteger modifierXor;

			try {
				
				/* Use SHA-256 for the scramble. */
				MessageDigest digest = MessageDigest.getInstance("SHA-256");
				digest.update(Long.toString(wId).getBytes());
				modifierXor = new BigInteger(digest.digest());
				
			} catch (NoSuchAlgorithmException e) {
				
				throw new SeedException(e.getMessage(), e);
				
			}

			/* Xor the final seed with the modifierXor. */
			finalSeed = finalSeed.xor(modifierXor);
			
		}

		/* Keep increasing the final seed deterministically until it has
		 * at least the size required by the RNG. */
		while (finalSeed.bitCount() < length * 8) {
			
			/* We add the value 11 for the case when the base seed is zero. */
			finalSeed = finalSeed.pow(2).add(BigInteger.TEN).add(BigInteger.ONE);
			
		}

		/* Return a byte array of the exact length required by the RNG. */
		return Arrays.copyOf(finalSeed.toByteArray(), length);
	}
	
}

