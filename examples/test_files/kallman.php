<?php

class KalmanFilter 
{
    private float $r; // Process noise covariance
    private float $q; // Measurement noise covariance
    private float $a; // State transition matrix
    private float $b; // Control matrix
    private float $c; // Measurement matrix
    
    private float $Cov;
    private ?float $x = null; // Estimated signal value

    /**
     * KalmanFilter Constructor
     * 
     * @param float $r Process noise (how much we expect the system to wiggle on its own)
     * @param float $q Measurement noise (how much jitter/noise your sensor has)
     * @param float $a State vector
     * @param float $b Control vector
     * @param float $c Measurement vector
     */
    public function __construct(
        float $r = 1, 
        float $q = 1, 
        float $a = 1, 
        float $b = 0, 
        float $c = 1
    ) {
        $this->r = $r;
        $this->q = $q;
        $this->a = $a;
        $this->b = $b;
        $this->c = $c;


        $this->Cov = 0;
        
    }

    /**
     * Filters a new noisy measurement to return a clean prediction.
     * 
     * @param float $z The raw measurement value
     * @param float $u Optional control vector input
     * @return float The estimated/filtered value
     */
    public function filter(float $z, float $u = 0): float 
    {
        // 1. Initialize state if this is the first execution

        if (is_null($this->x)) {

            $this->x = (1 / $this->c) * $z;
            $this->Cov = (1 / $this->c) * $this->q * (1 / $this->c);

            return $this->x;
        }

        // 2. Prediction step
        $predX = ($this->a * $this->x) + ($this->b * $u);
        $predCov = (($this->a * $this->Cov) * $this->a) + $this->r;

        // 3. Update / Correction step
        $kalmanGain = $predCov * $this->c * (1 / (($this->c * $predCov * $this->c) + $this->q));
        
        $this->x = $predX + ($kalmanGain * ($z - ($this->c * $predX)));

        $this->Cov = $predCov - ($kalmanGain * $this->c * $predCov);

        return $this->x;
    }

    /**
     * Get the current state estimate
     */
    public function getState(): ?float 
    {
        return $this->x;
    }
}


// Tuning parameters:
// R (Process Noise) = 0.01: We assume the true value is mostly stable.
// Q (Measurement Noise) = 1.00: We trust our raw data less because it's noisy.
$filter = new KalmanFilter(0.01, 1.00/*, 1.0, 0.0, 1.0*/);

// Simulated noisy incoming data stream
$noisyData = array(10.5, 9.2, 11.1, 8.9, 10.8, 9.5, 10.1, 11.5, 9.0, 10.2);

echo "Raw Value -> Filtered Value\n";
echo "---------------------------\n";

foreach ($noisyData as $reading) {
    $corrected = $filter->filter($reading);
    printf("%6.2f    -> %6.4f\n", $reading, $corrected);
}
