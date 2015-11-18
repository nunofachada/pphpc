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

import org.uncommons.maths.random.AESCounterRNG;
import org.uncommons.maths.random.CMWC4096RNG;
import org.uncommons.maths.random.CellularAutomatonRNG;
import org.uncommons.maths.random.JavaRNG;
import org.uncommons.maths.random.MersenneTwisterRNG;
import org.uncommons.maths.random.SeedGenerator;
import org.uncommons.maths.random.XORShiftRNG;

/**
 * Enum representing the random number generators present in the Uncommons
 * Math library.
 * 
 * @author Nuno Fachada
 */
public enum RNGType {
	
	/** @see org.uncommons.maths.random.AESCounterRNG */
	AES {
		@Override
		public Random createRNG(SeedGenerator seedGen) throws Exception {
			return new AESCounterRNG(seedGen);
		}
	},
	/** @see org.uncommons.maths.random.CellularAutomatonRNG */
	CA {
		@Override
		public Random createRNG(SeedGenerator seedGen) throws Exception {
			return new CellularAutomatonRNG(seedGen);
		}
	}, 
	/** @see org.uncommons.maths.random.CMWC4096RNG */
	CMWC {
		@Override
		public Random createRNG(SeedGenerator seedGen) throws Exception {
			return new CMWC4096RNG(seedGen);
		}
	}, 
	/** @see org.uncommons.maths.random.JavaRNG */
	JAVA {
		@Override
		public Random createRNG(SeedGenerator seedGen) throws Exception {
			return new JavaRNG(seedGen);
		}
	}, 
	/** @see org.uncommons.maths.random.MersenneTwisterRNG */
	MT {
		@Override
		public Random createRNG(SeedGenerator seedGen) throws Exception {
			return new MersenneTwisterRNG(seedGen);
		}
	},
	/** @see RanduRNG */
	RANDU {
		@Override
		public Random createRNG(SeedGenerator seedGen) throws Exception {
			return new RanduRNG(seedGen);
		}
	},
	/** @see ModMidSquareRNG */
	MODMIDSQUARE {
		@Override
		public Random createRNG(SeedGenerator seedGen) throws Exception {
			return new ModMidSquareRNG(seedGen);
		}
	},
	/** @see org.uncommons.maths.random.XORShiftRNG */
	XORSHIFT {
		@Override
		public Random createRNG(SeedGenerator seedGen) throws Exception {
			return new XORShiftRNG(seedGen);
		}
	};
	
	/**
	 * Create the random number generator associated with this RNG type.
	 * 
	 * @param seedGen Seed generator.
	 * @return A random number generator associated with this RNG type.
	 * @throws Exception If some problem occurs while creating the RNG.
	 */
	public abstract Random createRNG(SeedGenerator seedGen) throws Exception;

}
