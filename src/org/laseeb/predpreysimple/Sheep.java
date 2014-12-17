package org.laseeb.predpreysimple;

/**
 * Sheep class.
 * @author Nuno Fachada
 *
 */
public class Sheep extends Agent {

	/**
	 * Constructor.
	 * @param energy Initial agents' energy.
	 */
	public Sheep(int energy) {
		super(energy);
	}
	
	@Override
	/**
	 * @see Agent
	 */
	protected void play(Cell cell) {
		if (cell.getGrass() == 0) {
			cell.eatGrass();
			this.setEnergy(this.getEnergy() + PredPreySimple.SHEEP_GAIN_FROM_FOOD);
		}

	}

	@Override
	/**
	 * @see Agent
	 */
	protected int getReproduceProbability() {
		return PredPreySimple.SHEEP_REPRODUCE_PROB;
	}

	@Override
	/**
	 * @see Agent
	 */
	protected int getReproduceThreshold() {
		return PredPreySimple.SHEEP_REPRODUCE_THRESHOLD;
	}

}
