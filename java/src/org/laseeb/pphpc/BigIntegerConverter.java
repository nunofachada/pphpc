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

import java.math.BigInteger;

import com.beust.jcommander.ParameterException;
import com.beust.jcommander.converters.BaseConverter;

/**
 * This class provides a String to BigInteger converter for JCommander,
 * which allows the user to pass a big integer as a command line option.
 * 
 * @author Nuno Fachada
 */
public class BigIntegerConverter extends BaseConverter<BigInteger> {

	/**
	 * Create a new BigInteger converter.
	 * 
	 * @param optionName Name of command-line option which accepting a BigInteger.
	 */
	public BigIntegerConverter(String optionName) {
		super(optionName);
	}

	/**
	 * @see com.beust.jcommander.converters.BaseConverter#convert(String)
	 */
	@Override
	public BigInteger convert(String value) {
		try {
			return new BigInteger(value);
		} catch (NumberFormatException nfe) {
			throw new ParameterException(this.getErrorString(value, "a BigInteger"));
		}
		
	}

}
