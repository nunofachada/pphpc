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

import java.io.IOException;
import java.math.BigInteger;
import java.util.Random;

import org.uncommons.maths.random.MersenneTwisterRNG;
import org.uncommons.maths.random.SeedException;

/**
 * Small class to test the available RNGs with Dieharder.
 * 
 * Usage: java -cp bin:lib/* org.laseeb.pphpc.TestRNGs | dieharder -g 200 -a
 * 
 * @author Nuno Fachada
 */
public class TestRNGs {

	public static void main(String[] args) {
		byte[] seed = { 3, 4 , 5 ,6, 7 ,8, 8 ,9};
		ModelSeedGenerator sg = new ModelSeedGenerator(1, new BigInteger(seed));
		Random rng = null;
		try {
			rng = new ReallyPoorRNG(sg);
//			rng = new MersenneTwisterRNG(sg);
		} catch (SeedException e) {
			e.printStackTrace();
		}
		
		byte[] bytes = new byte[4];
		while (true) {
			rng.nextBytes(bytes);
			try {
				System.out.write(bytes);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

}