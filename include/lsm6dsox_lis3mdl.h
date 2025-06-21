class Lsm6dsoxLis3mdl
{
public:
    template <typename T> struct vector
    {
      T x, y, z;
    };

    bool Init();
    vector<int> GetAcceleratorRawValues();
    void CalculateAcceleratorAveBias();
    vector<double> GetAccelatorValues();
    vector<double> GetAcceleratorAngles();
    vector<int> GetGyroRawValues();
    void CalculateGyroAveBias();
    vector<double> GetGyroValues();
    vector<double> GetGyroAngles(int storeValueStepMs, vector<double> lastGyroAngles);
    vector<int> GetCompassRawValues();
    void CalculateCompassMinMax();
    vector<double> HardAndSoftIronCorrectedHeading(vector<int> rawValues);
    vector<double> GetCompassValues();
    vector<double> GetCompassValuesHSCorrected();

    vector<double> lastGyroAngles;
   private:
    vector<int> accelRawBias;
    vector<int> gyroRawBias;
    vector<int> lastGyroRawValues;
    vector<int> lastAccelRawValues;
};