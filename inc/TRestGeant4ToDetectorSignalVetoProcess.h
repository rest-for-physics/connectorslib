
#include <TRestDetectorSignalEvent.h>
#include <TRestEventProcess.h>
#include <TRestGeant4Event.h>
#include <TRestGeant4Metadata.h>

#ifndef RestCore_TRestGeant4ToRawSignalVetoProcess
#define RestCore_TRestGeant4ToRawSignalVetoProcess

class TRestGeant4ToDetectorSignalVetoProcess : public TRestEventProcess {
   private:
    TString fVetoVolumesExpression;
    TString fVetoDetectorsExpression;
    double fVetoDetectorOffsetSize = 0;
    double fVetoLightAttenuation = 0;
    double fVetoQuenchingFactor = 1.0;

   public:
    inline TString GetVetoVolumesExpression() const { return fVetoVolumesExpression; }
    inline TString GetVetoDetectorExpression() const { return fVetoDetectorsExpression; }
    inline double GetVetoDetectorOffsetSize() const { return fVetoDetectorOffsetSize; }
    inline double GetVetoLightAttenuation() const { return fVetoLightAttenuation; }
    inline double GetVetoQuenchingFactor() const { return fVetoQuenchingFactor; }

    inline void SetVetoVolumesExpression(const TString& expression) { fVetoVolumesExpression = expression; }
    inline void SetVetoDetectorsExpression(const TString& expression) {
        fVetoDetectorsExpression = expression;
    }
    inline void SetVetoDetectorOffsetSize(double offset) { fVetoDetectorOffsetSize = offset; }
    inline void SetVetoLightAttenuation(double attenuation) { fVetoLightAttenuation = attenuation; }
    inline void SetVetoQuenchingFactor(double quenchingFactor) { fVetoQuenchingFactor = quenchingFactor; }

   private:
    TRestGeant4Event* fInputEvent = nullptr;               //!
    TRestDetectorSignalEvent* fOutputEvent = nullptr;      //!
    const TRestGeant4Metadata* fGeant4Metadata = nullptr;  //!

    std::vector<TString> fVetoVolumes;
    std::vector<TString> fVetoDetectorVolumes;
    std::map<TString, TVector3> fVetoDetectorBoundaryPosition;
    std::map<TString, TVector3> fVetoDetectorBoundaryDirection;

    void InitFromConfigFile() override;
    void Initialize() override;
    void LoadDefaultConfig();

   public:
    any GetInputEvent() const override { return fInputEvent; }
    any GetOutputEvent() const override { return fOutputEvent; }

    inline void SetGeant4Metadata(const TRestGeant4Metadata* metadata) {
        fGeant4Metadata = metadata;
    }  // TODO: We should not need this! but `GetMetadata<TRestGeant4Metadata>()` is not working early in the
       // processing (look at the tests for more details)

    void InitProcess() override;
    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;
    void EndProcess() override;

    void LoadConfig(const std::string& configFilename, const std::string& name = "");

    void PrintMetadata() override;

    const char* GetProcessName() const override { return "Geant4ToDetectorSignalVetoProcess"; }

    TRestGeant4ToDetectorSignalVetoProcess();
    TRestGeant4ToDetectorSignalVetoProcess(const char* configFilename);
    ~TRestGeant4ToDetectorSignalVetoProcess();

    ClassDefOverride(TRestGeant4ToDetectorSignalVetoProcess, 1);
};

#endif  // RestCore_TRestGeant4ToRawSignalVetoProcess
