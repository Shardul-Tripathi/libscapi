#include "../../include/interactive_mid_protocols/SigmaProtocolDamgardJurikEncryptedZero.hpp"

/**
* Constructor that gets the soundness parameter, length parameter and SecureRandom.
* @param t Soundness parameter in BITS.
* @param lengthParameter length parameter in BITS.
* @param random
*/
SigmaDJEncryptedZeroSimulator::SigmaDJEncryptedZeroSimulator(int t, int lengthParameter) {
	this->t = t;
	this->lengthParameter = lengthParameter;
	random = get_seeded_random();
}

/**
* Computes the simulator computation with the given challenge.
* @param input MUST be an instance of SigmaDJEncryptedZeroCommonInput.
* @param challenge
* @return the output of the computation - (a, e, z).
* @throws CheatAttemptException if the received challenge's length is not equal to the soundness parameter.
* @throws IllegalArgumentException if the given input is not an instance of SigmaDJEncryptedZeroCommonInput.
*/
shared_ptr<SigmaSimulatorOutput> SigmaDJEncryptedZeroSimulator::simulate(SigmaCommonInput* input, vector<byte> challenge) {
	//check the challenge validity.
	if (!checkChallengeLength(challenge)) {
		throw CheatAttemptException("the length of the given challenge is differ from the soundness parameter");
	}

	auto djInput = dynamic_cast<SigmaDJEncryptedZeroCommonInput*>(input);
	if (djInput == NULL) {
		throw invalid_argument("the given input must be an instance of SigmaDJEncryptedZeroInput");
	}
	
	biginteger n = djInput->getPublicKey().getModulus();
	//Check the soundness validity.
	if (!checkSoundnessParam(n)) {
		throw invalid_argument("t must be less than a third of the length of the public key n");
	}

	//Sample a random value z <- Z*n
	biginteger z = getRandomInRange(1, n - 1, random);

	//Calculate N = n^s and N' = n^(s+1)
	biginteger N = mp::pow(n, lengthParameter);
	biginteger NTag = mp::pow(n, lengthParameter + 1);
	biginteger e = decodeBigInteger(challenge.data(), challenge.size());

	//Compute a = z^N/c^e mod N'
	biginteger zToN = mp::powm(z, N, NTag);
	biginteger denominator = mp::powm(djInput->getCiphertext().getCipher(), e, NTag);
	biginteger denomInv = MathAlgorithms::modInverse(denominator, NTag);
	biginteger a = (zToN * denomInv) % NTag;

	//Output (a,e,z).
	return make_shared<SigmaSimulatorOutput>(make_shared<SigmaBIMsg>(a), challenge, make_shared<SigmaBIMsg>(z));

}

/**
* Computes the simulator computation with a randomly chosen challenge.
* @param input MUST be an instance of SigmaDJEncryptedZeroInput.
* @return the output of the computation - (a, e, z).
* @throws IllegalArgumentException if the given input is not an instance of SigmaDJEncryptedZeroInput.
*/
shared_ptr<SigmaSimulatorOutput> SigmaDJEncryptedZeroSimulator::simulate(SigmaCommonInput* input)  {
	//Create a new byte array of size t/8, to get the required byte size and fill it with random values.
	vector<byte> e;
	gen_random_bytes_vector(e, t / 8, random);
	
	// call the other simulate function with the given input and the sampled e.
	return simulate(input, e);
}

/**
* Checks the validity of the given soundness parameter.<p>
* t must be less than a third of the length of the public key n.
* @return true if the soundness parameter is valid; false, otherwise.
*/
bool SigmaDJEncryptedZeroSimulator::checkSoundnessParam(biginteger modulus) {
	//If soundness parameter is not less than a third of the publicKey n, return false.
	int third = NumberOfBits(modulus) / 3;
	return (t < third);
}

/**
* Checks if the given challenge length is equal to the soundness parameter.
* @return true if the challenge length is t; false, otherwise.
*/
bool SigmaDJEncryptedZeroSimulator::checkChallengeLength(vector<byte> challenge) {
	//If the challenge's length is equal to t, return true. else, return false.
	return (challenge.size() == (t / 8) ? true : false);
}

/**
* Constructor that gets the soundness parameter, length parameter and SecureRandom.
* @param t Soundness parameter in BITS.
* @param lengthParameter length parameter in BITS.
* @param random
*/
SigmaDJEncryptedZeroProverComputation::SigmaDJEncryptedZeroProverComputation(int t, int lengthParameter) {
	this->t = t;
	this->lengthParameter = lengthParameter;
	random = get_seeded_random();
}

/**
* Checks the validity of the given soundness parameter.<p>
* t must be less than a third of the length of the public key n.
* @return true if the soundness parameter is valid; false, otherwise.
*/
bool SigmaDJEncryptedZeroProverComputation::checkSoundnessParam(biginteger modulus) {
	//If soundness parameter is not less than a third of the publicKey n, return false.
	int third = NumberOfBits(modulus) / 3;
	return (t < third);
}

/**
* Computes the first message of the protocol.<p>
* "SAMPLE random value s <- Z*n<p>
* COMPUTE a = s^N mod N'".
* @param input MUST be an instance of SigmaDJEncryptedZeroProverInput.
* @return the computed message
* @throws IllegalArgumentException if input is not an instance of SigmaDJEncryptedZeroProverInput.
*/
shared_ptr<SigmaProtocolMsg> SigmaDJEncryptedZeroProverComputation::computeFirstMsg(shared_ptr<SigmaProverInput> input) {
	this->input = dynamic_pointer_cast<SigmaDJEncryptedZeroProverInput>(input);
	if (this->input == NULL) {
		throw invalid_argument("the given input must be an instance of SigmaDJEncryptedZeroProverInput");
	}

	n = dynamic_pointer_cast<SigmaDJEncryptedZeroCommonInput>(this->input->getCommonInput())->getPublicKey().getModulus();
	//Check the soundness validity.
	if (!checkSoundnessParam(n)) {
		throw invalid_argument("t must be less than a third of the length of the public key n");
	}

	//Sample s in Z*n
	s = getRandomInRange(1, n - 1, random);
	
	//Calculate N = n^s and N' = n^(s+1)
	biginteger N = mp::pow(n, lengthParameter);
	biginteger NTag = mp::pow(n, lengthParameter + 1);

	//Compute a = s^N mod N'.
	biginteger a = mp::powm(s, N, NTag);
	
	//Create and return SigmaBIMsg with a.
	return make_shared<SigmaBIMsg>(a);
}

/**
* Computes the second message of the protocol.<p>
* "COMPUTE z = s*r^e mod n".
* @param challenge
* @return the computed message.
* @throws CheatAttemptException if the received challenge's length is not equal to the soundness parameter.
*/
shared_ptr<SigmaProtocolMsg> SigmaDJEncryptedZeroProverComputation::computeSecondMsg(vector<byte> challenge) {

	//check the challenge validity.
	if (!checkChallengeLength(challenge)) {
		throw CheatAttemptException("the length of the given challenge is differ from the soundness parameter");
	}

	//Compute z = (s*r^e) mod n
	biginteger e = decodeBigInteger(challenge.data(), challenge.size());
	biginteger rToe = mp::powm(input->getR(), e, n);
	biginteger z = (s * rToe) % n;

	//Delete the random value r
	s = 0;

	//Create and return SigmaBIMsg with z.
	return make_shared<SigmaBIMsg>(z);

}

/**
* Checks the validity of the given soundness parameter. <p>
* t must be less than a third of the length of the public key n.
* @return true if the soundness parameter is valid; false, otherwise.
*/
bool SigmaDJEncryptedZeroVerifierComputation::checkSoundnessParam(biginteger modulus) {
	//If soundness parameter is not less than a third of the publicKey n, throw IllegalArgumentException.
	int third = NumberOfBits(modulus) / 3;
	return (t < third);
}

/**
* Constructor that gets the soundness parameter, length parameter and SecureRandom.
* @param t Soundness parameter in BITS.
* @param lengthParameter length parameter in BITS.
* @param random
*/
SigmaDJEncryptedZeroVerifierComputation::SigmaDJEncryptedZeroVerifierComputation(int t, int lengthParameter) {

	this->t = t;
	this->lengthParameter = lengthParameter;
	random = get_seeded_random();
}

/**
* Samples the challenge of the protocol.<p>
* 	"SAMPLE a random challenge e<-{0,1}^t".
*/
void SigmaDJEncryptedZeroVerifierComputation::sampleChallenge() {
	//Create a new byte array of size t/8, to get the required byte size.
	gen_random_bytes_vector(e, t / 8, random);
}

/**
* Computes the verification of the protocol.<p>
* 	"ACC IFF c,a,z are relatively prime to n AND z^N = (a*c^e) mod N'".
* @param input MUST be an instance of SigmaDJEncryptedZeroCommonInput.
* @param z second message from prover
* @return true if the proof has been verified; false, otherwise.
* @throws IllegalArgumentException if input is not an instance of SigmaDJEncryptedZeroCommonInput.
* @throws IllegalArgumentException if the one of the prover's messages are not an instance of SigmaBIMsg
*/
bool SigmaDJEncryptedZeroVerifierComputation::verify(SigmaCommonInput* input, SigmaProtocolMsg* a, SigmaProtocolMsg* z) {
	auto djInput = dynamic_cast<SigmaDJEncryptedZeroCommonInput*>(input);
	if (djInput == NULL) {
		throw invalid_argument("the given input must be an instance of SigmaDJEncryptedZeroCommonInput");
	}

	n = djInput->getPublicKey().getModulus();
	//Check the soundness validity.
	if (!checkSoundnessParam(n)) {
		throw invalid_argument("t must be less than a third of the length of the public key n");
	}

	bool verified = true;
	auto aV = dynamic_cast<SigmaBIMsg*>(a);
	auto zV = dynamic_cast<SigmaBIMsg*>(z);
	//If one of the messages is illegal, throw exception.
	if (aV == NULL) {
		throw invalid_argument("first message must be an instance of SigmaBIMsg");
	}
	if (zV == NULL) {
		throw invalid_argument("second message must be an instance of SigmaBIMsg");
	}

	//Get the exponent in the second message from the prover.
	biginteger zBI = zV->getMsg();
	//Get the exponent in the second message from the prover.
	biginteger aBI = aV->getMsg();
	
	//Get the cipher value.
	biginteger c = djInput->getCiphertext().getCipher();

	//If a is not relatively prime to n, set verified to false.
	verified = verified && (mp::gcd(aBI, n) == 1);

	//If z is not relatively prime to n, set verified to false.
	verified = verified && (mp::gcd(zBI, n) == 1);

	//If c is not relatively prime to n, set verified to false.
	verified = verified && (mp::gcd(c, n) == 1);
	
	//Calculate N = n^s and N' = n^(s+1)
	biginteger N = mp::pow(n, lengthParameter);
	biginteger NTag = mp::pow(n, lengthParameter + 1);

	//Calculate z^N mod N' (left side of the equation).
	biginteger left = mp::powm(zBI, N, NTag);
	//Calculate (a*c^e) mod N' (left side of the equation).
	//Convert e to BigInteger.
	biginteger eBI = decodeBigInteger(e.data(), e.size());
	biginteger cToe = mp::powm(c, eBI, NTag);
	biginteger right = (aBI* cToe) % NTag;
	
	//If left and right sides of the equation are not equal, set verified to false.
	verified = verified && left == right;

	e.clear(); //Delete the random value e.

	//Return true if all checks returned true; false, otherwise.
	return verified;
}