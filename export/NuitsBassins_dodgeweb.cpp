/*******************************************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER $200k in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below $200k annual revenue or funding.

For entities with OVER $200k in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing@cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/10730637742483-RNBO-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
*******************************************************************************************************************/

#ifdef RNBO_LIB_PREFIX
#define STR_IMPL(A) #A
#define STR(A) STR_IMPL(A)
#define RNBO_LIB_INCLUDE(X) STR(RNBO_LIB_PREFIX/X)
#else
#define RNBO_LIB_INCLUDE(X) #X
#endif // RNBO_LIB_PREFIX
#ifdef RNBO_INJECTPLATFORM
#define RNBO_USECUSTOMPLATFORM
#include RNBO_INJECTPLATFORM
#endif // RNBO_INJECTPLATFORM

#include RNBO_LIB_INCLUDE(RNBO_Common.h)
#include RNBO_LIB_INCLUDE(RNBO_AudioSignal.h)

namespace RNBO {


#define trunc(x) ((Int)(x))
#define autoref auto&

#if defined(__GNUC__) || defined(__clang__)
    #define RNBO_RESTRICT __restrict__
#elif defined(_MSC_VER)
    #define RNBO_RESTRICT __restrict
#endif

#define FIXEDSIZEARRAYINIT(...) { }

template <class ENGINE = INTERNALENGINE> class rnbomatic : public PatcherInterfaceImpl {

friend class EngineCore;
friend class Engine;
friend class MinimalEngine<>;
public:

rnbomatic()
: _internalEngine(this)
{
}

~rnbomatic()
{
    deallocateSignals();
}

Index getNumMidiInputPorts() const {
    return 0;
}

void processMidiEvent(MillisecondTime , int , ConstByteArray , Index ) {}

Index getNumMidiOutputPorts() const {
    return 0;
}

void process(
    const SampleValue * const* inputs,
    Index numInputs,
    SampleValue * const* outputs,
    Index numOutputs,
    Index n
) {
    RNBO_UNUSED(numInputs);
    RNBO_UNUSED(inputs);
    this->vs = n;
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
    SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
    SampleValue * out2 = (numOutputs >= 2 && outputs[1] ? outputs[1] : this->dummyBuffer);

    this->groove_01_perform(
        this->groove_01_rate_auto,
        this->groove_01_begin,
        this->groove_01_end,
        this->signals[0],
        this->dummyBuffer,
        n
    );

    this->groove_02_perform(
        this->groove_02_rate_auto,
        this->groove_02_begin,
        this->groove_02_end,
        this->signals[1],
        this->dummyBuffer,
        n
    );

    this->groove_03_perform(
        this->groove_03_rate_auto,
        this->groove_03_begin,
        this->groove_03_end,
        this->signals[2],
        this->dummyBuffer,
        n
    );

    this->signaladder_01_perform(this->signals[0], this->signals[1], this->signals[2], out2, n);
    this->signaladder_02_perform(this->signals[0], this->signals[1], this->signals[2], out1, n);
    this->stackprotect_perform(n);
    this->globaltransport_advance();
    this->advanceTime((ENGINE*)nullptr);
    this->audioProcessSampleCount += this->vs;
}

void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
    RNBO_ASSERT(this->_isInitialized);

    if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
        Index i;

        for (i = 0; i < 3; i++) {
            this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
        }

        this->globaltransport_tempo = resizeSignal(this->globaltransport_tempo, this->maxvs, maxBlockSize);
        this->globaltransport_state = resizeSignal(this->globaltransport_state, this->maxvs, maxBlockSize);
        this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
        this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
        this->didAllocateSignals = true;
    }

    const bool sampleRateChanged = sampleRate != this->sr;
    const bool maxvsChanged = maxBlockSize != this->maxvs;
    const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;

    if (sampleRateChanged || maxvsChanged) {
        this->vs = maxBlockSize;
        this->maxvs = maxBlockSize;
        this->sr = sampleRate;
        this->invsr = 1 / sampleRate;
    }

    this->groove_01_dspsetup(forceDSPSetup);
    this->groove_02_dspsetup(forceDSPSetup);
    this->groove_03_dspsetup(forceDSPSetup);
    this->data_01_dspsetup(forceDSPSetup);
    this->data_02_dspsetup(forceDSPSetup);
    this->data_03_dspsetup(forceDSPSetup);
    this->globaltransport_dspsetup(forceDSPSetup);

    if (sampleRateChanged)
        this->onSampleRateChanged(sampleRate);
}

number msToSamps(MillisecondTime ms, number sampleRate) {
    return ms * sampleRate * 0.001;
}

MillisecondTime sampsToMs(SampleIndex samps) {
    return samps * (this->invsr * 1000);
}

Index getNumInputChannels() const {
    return 0;
}

Index getNumOutputChannels() const {
    return 2;
}

DataRef* getDataRef(DataRefIndex index)  {
    switch (index) {
    case 0:
        {
        return addressOf(this->bouclier00);
        break;
        }
    case 1:
        {
        return addressOf(this->bouclier01);
        break;
        }
    case 2:
        {
        return addressOf(this->bouclier02);
        break;
        }
    default:
        {
        return nullptr;
        }
    }
}

DataRefIndex getNumDataRefs() const {
    return 3;
}

void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (index == 0) {
        this->groove_01_buffer = reInitDataView(this->groove_01_buffer, this->bouclier00);
        this->data_01_buffer = reInitDataView(this->data_01_buffer, this->bouclier00);
        this->data_01_bufferUpdated();
    }

    if (index == 1) {
        this->groove_02_buffer = reInitDataView(this->groove_02_buffer, this->bouclier01);
        this->data_02_buffer = reInitDataView(this->data_02_buffer, this->bouclier01);
        this->data_02_bufferUpdated();
    }

    if (index == 2) {
        this->groove_03_buffer = reInitDataView(this->groove_03_buffer, this->bouclier02);
        this->data_03_buffer = reInitDataView(this->data_03_buffer, this->bouclier02);
        this->data_03_bufferUpdated();
    }
}

void initialize() {
    RNBO_ASSERT(!this->_isInitialized);

    this->bouclier00 = initDataRef(
        this->bouclier00,
        this->dataRefStrings->name0,
        false,
        this->dataRefStrings->file0,
        this->dataRefStrings->tag0
    );

    this->bouclier01 = initDataRef(
        this->bouclier01,
        this->dataRefStrings->name1,
        false,
        this->dataRefStrings->file1,
        this->dataRefStrings->tag1
    );

    this->bouclier02 = initDataRef(
        this->bouclier02,
        this->dataRefStrings->name2,
        false,
        this->dataRefStrings->file2,
        this->dataRefStrings->tag2
    );

    this->assign_defaults();
    this->applyState();
    this->bouclier00->setIndex(0);
    this->groove_01_buffer = new Float32Buffer(this->bouclier00);
    this->data_01_buffer = new Float32Buffer(this->bouclier00);
    this->bouclier01->setIndex(1);
    this->groove_02_buffer = new Float32Buffer(this->bouclier01);
    this->data_02_buffer = new Float32Buffer(this->bouclier01);
    this->bouclier02->setIndex(2);
    this->groove_03_buffer = new Float32Buffer(this->bouclier02);
    this->data_03_buffer = new Float32Buffer(this->bouclier02);
    this->initializeObjects();
    this->allocateDataRefs();
    this->startup();
    this->_isInitialized = true;
}

void getPreset(PatcherStateInterface& preset) {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    preset["__presetid"] = "rnbo";
}

void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
    this->updateTime(time, (ENGINE*)nullptr);
}

void setParameterValue(ParameterIndex , ParameterValue , MillisecondTime ) {}

void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValue(index, value, time);
}

void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
    this->setParameterValue(index, this->getParameterValue(index), time);
}

void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValueNormalized(index, value, time);
}

ParameterValue getParameterValue(ParameterIndex index)  {
    switch (index) {
    default:
        {
        return 0;
        }
    }
}

ParameterIndex getNumSignalInParameters() const {
    return 0;
}

ParameterIndex getNumSignalOutParameters() const {
    return 0;
}

ParameterIndex getNumParameters() const {
    return 0;
}

ConstCharPointer getParameterName(ParameterIndex index) const {
    switch (index) {
    default:
        {
        return "bogus";
        }
    }
}

ConstCharPointer getParameterId(ParameterIndex index) const {
    switch (index) {
    default:
        {
        return "bogus";
        }
    }
}

void getParameterInfo(ParameterIndex , ParameterInfo * ) const {}

ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
    if (steps == 1) {
        if (normalizedValue > 0) {
            normalizedValue = 1.;
        }
    } else {
        ParameterValue oneStep = (number)1. / (steps - 1);
        ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
        normalizedValue = numberOfSteps * oneStep;
    }

    return normalizedValue;
}

ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    default:
        {
        return value;
        }
    }
}

ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

    switch (index) {
    default:
        {
        return value;
        }
    }
}

ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    default:
        {
        return value;
        }
    }
}

void processNumMessage(MessageTag tag, MessageTag objectId, MillisecondTime time, number payload) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("bouclier"):
        {
        this->inport_01_value_number_set(payload);
        break;
        }
    case TAG("listin"):
        {
        if (TAG("message_obj-44") == objectId)
            this->message_01_listin_number_set(payload);

        break;
        }
    }
}

void processListMessage(
    MessageTag tag,
    MessageTag objectId,
    MillisecondTime time,
    const list& payload
) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("bouclier"):
        {
        this->inport_01_value_list_set(payload);
        break;
        }
    case TAG("listin"):
        {
        if (TAG("message_obj-44") == objectId)
            this->message_01_listin_list_set(payload);

        break;
        }
    }
}

void processBangMessage(MessageTag tag, MessageTag objectId, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (tag) {
    case TAG("bangin"):
        {
        if (TAG("button_obj-41") == objectId)
            this->button_01_bangin_bang();

        if (TAG("button_obj-43") == objectId)
            this->button_02_bangin_bang();

        if (TAG("button_obj-45") == objectId)
            this->button_03_bangin_bang();

        break;
        }
    case TAG("bouclier"):
        {
        this->inport_01_value_bang_bang();
        break;
        }
    case TAG("listin"):
        {
        if (TAG("message_obj-44") == objectId)
            this->message_01_listin_bang_bang();

        break;
        }
    }
}

MessageTagInfo resolveTag(MessageTag tag) const {
    switch (tag) {
    case TAG("bangout"):
        {
        return "bangout";
        }
    case TAG("button_obj-41"):
        {
        return "button_obj-41";
        }
    case TAG("button_obj-43"):
        {
        return "button_obj-43";
        }
    case TAG("button_obj-45"):
        {
        return "button_obj-45";
        }
    case TAG("listout"):
        {
        return "listout";
        }
    case TAG("message_obj-44"):
        {
        return "message_obj-44";
        }
    case TAG("outie"):
        {
        return "outie";
        }
    case TAG(""):
        {
        return "";
        }
    case TAG("bangin"):
        {
        return "bangin";
        }
    case TAG("bouclier"):
        {
        return "bouclier";
        }
    case TAG("listin"):
        {
        return "listin";
        }
    }

    return "";
}

MessageIndex getNumMessages() const {
    return 2;
}

const MessageInfo& getMessageInfo(MessageIndex index) const {
    switch (index) {
    case 0:
        {
        static const MessageInfo r0 = {
            "outie",
            Outport
        };

        return r0;
        }
    case 1:
        {
        static const MessageInfo r1 = {
            "bouclier",
            Inport
        };

        return r1;
        }
    }

    return NullMessageInfo;
}

protected:

		
void advanceTime(EXTERNALENGINE*) {}
void advanceTime(INTERNALENGINE*) {
	_internalEngine.advanceTime(sampstoms(this->vs));
}

void processInternalEvents(MillisecondTime time) {
	_internalEngine.processEventsUntil(time);
}

void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
	if (time == TimeNow) time = getPatcherTime();
	processInternalEvents(inProcess ? time + sampsToMs(this->vs) : time);
	updateTime(time, (EXTERNALENGINE*)nullptr);
}

rnbomatic* operator->() {
    return this;
}
const rnbomatic* operator->() const {
    return this;
}
rnbomatic* getTopLevelPatcher() {
    return this;
}

void cancelClockEvents()
{
}

template<typename LISTTYPE = list> void listquicksort(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    if (l < h) {
        Int p = (Int)(this->listpartition(arr, sortindices, l, h, ascending));
        this->listquicksort(arr, sortindices, l, p - 1, ascending);
        this->listquicksort(arr, sortindices, p + 1, h, ascending);
    }
}

template<typename LISTTYPE = list> Int listpartition(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    number x = arr[(Index)h];
    Int i = (Int)(l - 1);

    for (Int j = (Int)(l); j <= h - 1; j++) {
        bool asc = (bool)((bool)(ascending) && arr[(Index)j] <= x);
        bool desc = (bool)((bool)(!(bool)(ascending)) && arr[(Index)j] >= x);

        if ((bool)(asc) || (bool)(desc)) {
            i++;
            this->listswapelements(arr, i, j);
            this->listswapelements(sortindices, i, j);
        }
    }

    i++;
    this->listswapelements(arr, i, h);
    this->listswapelements(sortindices, i, h);
    return i;
}

template<typename LISTTYPE = list> void listswapelements(LISTTYPE& arr, Int a, Int b) {
    auto tmp = arr[(Index)a];
    arr[(Index)a] = arr[(Index)b];
    arr[(Index)b] = tmp;
}

number minimum(number x, number y) {
    return (y < x ? y : x);
}

number maximum(number x, number y) {
    return (x < y ? y : x);
}

number mstosamps(MillisecondTime ms) {
    return ms * this->sr * 0.001;
}

MillisecondTime sampstoms(number samps) {
    return samps * 1000 / this->sr;
}

MillisecondTime getPatcherTime() const {
    return this->_currentTime;
}

void button_01_bangin_bang() {
    this->button_01_bangval_bang();
}

void inport_01_value_bang_bang() {
    this->inport_01_out_bang_bang();
}

void inport_01_value_number_set(number v) {
    this->inport_01_out_number_set(v);
}

template<typename LISTTYPE> void inport_01_value_list_set(const LISTTYPE& v) {
    this->inport_01_out_list_set(v);
}

void button_02_bangin_bang() {
    this->button_02_bangval_bang();
}

void button_03_bangin_bang() {
    this->button_03_bangval_bang();
}

template<typename LISTTYPE> void message_01_listin_list_set(const LISTTYPE& v) {
    this->message_01_set_set(v);
}

void message_01_listin_number_set(number v) {
    this->message_01_set_set(v);
}

void message_01_listin_bang_bang() {
    this->message_01_trigger_bang();
}

void deallocateSignals() {
    Index i;

    for (i = 0; i < 3; i++) {
        this->signals[i] = freeSignal(this->signals[i]);
    }

    this->globaltransport_tempo = freeSignal(this->globaltransport_tempo);
    this->globaltransport_state = freeSignal(this->globaltransport_state);
    this->zeroBuffer = freeSignal(this->zeroBuffer);
    this->dummyBuffer = freeSignal(this->dummyBuffer);
}

Index getMaxBlockSize() const {
    return this->maxvs;
}

number getSampleRate() const {
    return this->sr;
}

bool hasFixedVectorSize() const {
    return false;
}

void setProbingTarget(MessageTag ) {}

void fillDataRef(DataRefIndex , DataRef& ) {}

void zeroDataRef(DataRef& ref) {
    ref->setZero();
}

void allocateDataRefs() {
    this->groove_01_buffer = this->groove_01_buffer->allocateIfNeeded();
    this->data_01_buffer = this->data_01_buffer->allocateIfNeeded();

    if (this->bouclier00->hasRequestedSize()) {
        if (this->bouclier00->wantsFill())
            this->zeroDataRef(this->bouclier00);

        this->getEngine()->sendDataRefUpdated(0);
    }

    this->groove_02_buffer = this->groove_02_buffer->allocateIfNeeded();
    this->data_02_buffer = this->data_02_buffer->allocateIfNeeded();

    if (this->bouclier01->hasRequestedSize()) {
        if (this->bouclier01->wantsFill())
            this->zeroDataRef(this->bouclier01);

        this->getEngine()->sendDataRefUpdated(1);
    }

    this->groove_03_buffer = this->groove_03_buffer->allocateIfNeeded();
    this->data_03_buffer = this->data_03_buffer->allocateIfNeeded();

    if (this->bouclier02->hasRequestedSize()) {
        if (this->bouclier02->wantsFill())
            this->zeroDataRef(this->bouclier02);

        this->getEngine()->sendDataRefUpdated(2);
    }
}

void initializeObjects() {
    this->message_01_init();
    this->data_01_init();
    this->data_02_init();
    this->data_03_init();
}

Index getIsMuted()  {
    return this->isMuted;
}

void setIsMuted(Index v)  {
    this->isMuted = v;
}

void onSampleRateChanged(double ) {}

void extractState(PatcherStateInterface& ) {}

void applyState() {}

void processClockEvent(MillisecondTime , ClockId , bool , ParameterValue ) {}

void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}

void processOutletEvent(
    EngineLink* sender,
    OutletIndex index,
    ParameterValue value,
    MillisecondTime time
) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->processOutletAtCurrentTime(sender, index, value);
}

void sendOutlet(OutletIndex index, ParameterValue value) {
    this->getEngine()->sendOutlet(this, index, value);
}

void startup() {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    this->processParamInitEvents();
}

void groove_01_rate_bang_bang() {
    this->groove_01_changeIncomingInSamples = this->sampleOffsetIntoNextAudioBuffer + 1;
    this->groove_01_incomingChange = 1;
}

void button_01_output_bang() {
    this->groove_01_rate_bang_bang();
}

void button_01_bangval_bang() {
    ;
    this->button_01_output_bang();
}

template<typename LISTTYPE> void outport_01_input_list_set(const LISTTYPE& v) {
    this->getEngine()->sendListMessage(TAG("outie"), TAG(""), v, this->_currentTime);
}

template<typename LISTTYPE> void message_01_out_set(const LISTTYPE& v) {
    this->outport_01_input_list_set(v);
}

void message_01_trigger_bang() {
    if ((bool)(this->message_01_set->length) || (bool)(false)) {
        this->message_01_out_set(this->message_01_set);
    }
}

void route_01_nomatch_bang_bang() {}

void groove_03_rate_bang_bang() {
    this->groove_03_changeIncomingInSamples = this->sampleOffsetIntoNextAudioBuffer + 1;
    this->groove_03_incomingChange = 1;
}

void button_03_output_bang() {
    this->groove_03_rate_bang_bang();
}

void button_03_bangval_bang() {
    ;
    this->button_03_output_bang();
}

void route_01_match3_bang_bang() {
    this->button_03_bangval_bang();
}

void route_01_match3_number_set(number v) {
    RNBO_UNUSED(v);
    this->button_03_bangval_bang();
}

template<typename LISTTYPE> void route_01_match3_list_set(const LISTTYPE& v) {
    RNBO_UNUSED(v);
    this->button_03_bangval_bang();
}

void groove_02_rate_bang_bang() {
    this->groove_02_changeIncomingInSamples = this->sampleOffsetIntoNextAudioBuffer + 1;
    this->groove_02_incomingChange = 1;
}

void button_02_output_bang() {
    this->groove_02_rate_bang_bang();
}

void button_02_bangval_bang() {
    ;
    this->button_02_output_bang();
}

void route_01_match2_bang_bang() {
    this->button_02_bangval_bang();
}

void route_01_match2_number_set(number v) {
    RNBO_UNUSED(v);
    this->button_02_bangval_bang();
}

template<typename LISTTYPE> void route_01_match2_list_set(const LISTTYPE& v) {
    RNBO_UNUSED(v);
    this->button_02_bangval_bang();
}

void route_01_match1_bang_bang() {
    this->button_01_bangval_bang();
}

void route_01_match1_number_set(number v) {
    RNBO_UNUSED(v);
    this->button_01_bangval_bang();
}

template<typename LISTTYPE> void route_01_match1_list_set(const LISTTYPE& v) {
    RNBO_UNUSED(v);
    this->button_01_bangval_bang();
}

void route_01_nomatch_number_set(number ) {}

template<typename LISTTYPE> void route_01_nomatch_list_set(const LISTTYPE& ) {}

template<typename LISTTYPE> void route_01_input_list_set(const LISTTYPE& v) {
    if ((bool)(!(bool)(v->length)))
        this->route_01_nomatch_bang_bang();
    else {
        number check = v[0];
        list input = jsCreateListCopy(v);
        input->shift();
        bool nomatch = true;

        if (check == this->route_01_selector3) {
            if (input->length == 0)
                this->route_01_match3_bang_bang();
            else if (input->length == 1)
                this->route_01_match3_number_set(input[0]);
            else
                this->route_01_match3_list_set(input);

            nomatch = false;
        }

        if (check == this->route_01_selector2) {
            if (input->length == 0)
                this->route_01_match2_bang_bang();
            else if (input->length == 1)
                this->route_01_match2_number_set(input[0]);
            else
                this->route_01_match2_list_set(input);

            nomatch = false;
        }

        if (check == this->route_01_selector1) {
            if (input->length == 0)
                this->route_01_match1_bang_bang();
            else if (input->length == 1)
                this->route_01_match1_number_set(input[0]);
            else
                this->route_01_match1_list_set(input);

            nomatch = false;
        }

        if ((bool)(nomatch)) {
            if (v->length == 1) {
                this->route_01_nomatch_number_set(v[0]);
            } else {
                this->route_01_nomatch_list_set(v);
            }
        }
    }
}

void route_01_input_number_set(number v) {
    this->route_01_input_list_set(listbase<number, 1>{v});
}

void counter_01_output_set(number v) {
    this->counter_01_output = v;
    this->route_01_input_number_set(v);
}

void counter_01_overflow_bang_bang() {}

number counter_01_overflow_number_constrain(number v) const {
    if (v < 0)
        v = 0;

    if (v > 1)
        v = 1;

    return v;
}

void counter_01_overflow_number_set(number v) {
    v = this->counter_01_overflow_number_constrain(v);
    this->counter_01_overflow_number = v;
}

void counter_01_carry_set(number v) {
    this->counter_01_carry = v;
}

void counter_01_underflow_bang_bang() {}

number counter_01_underflow_number_constrain(number v) const {
    if (v < 0)
        v = 0;

    if (v > 1)
        v = 1;

    return v;
}

void counter_01_underflow_number_set(number v) {
    v = this->counter_01_underflow_number_constrain(v);
    this->counter_01_underflow_number = v;
}

void counter_01_input_bang() {
    this->counter_01_output_set(this->counter_01_count);
    this->counter_01_inc();

    if (this->counter_01_count > this->counter_01_maximum) {
        if (this->counter_01_direction == 2) {
            this->counter_01_phase = !(bool)(this->counter_01_phase);
            this->counter_01_count = this->counter_01_maximum;
            this->counter_01_inc();
        } else
            this->counter_01_count = this->counter_01_minimum;

        if ((bool)(!(bool)(this->counter_01_overflow_number))) {
            if (this->counter_01_carryflag == 1)
                this->counter_01_overflow_bang_bang();
            else
                this->counter_01_overflow_number_set(1);
        }

        this->counter_01_carry_set(this->counter_01_carry + 1);
    } else if ((bool)(this->counter_01_overflow_number) && this->counter_01_carryflag == 0)
        this->counter_01_overflow_number_set(0);

    if (this->counter_01_count < this->counter_01_minimum) {
        if (this->counter_01_direction == 2) {
            this->counter_01_phase = !(bool)(this->counter_01_phase);
            this->counter_01_count = this->counter_01_minimum;
            this->counter_01_inc();
        } else
            this->counter_01_count = this->counter_01_maximum;

        if ((bool)(!(bool)(this->counter_01_underflow_number))) {
            if (this->counter_01_carryflag == 1)
                this->counter_01_underflow_bang_bang();
            else
                this->counter_01_underflow_number_set(1);
        }

        this->counter_01_carry_set(this->counter_01_carry + 1);
    } else if ((bool)(this->counter_01_underflow_number) && this->counter_01_carryflag == 0)
        this->counter_01_underflow_number_set(0);
}

void inport_01_out_bang_bang() {
    this->message_01_trigger_bang();
    this->counter_01_input_bang();
}

void inport_01_out_number_set(number v) {
    RNBO_UNUSED(v);
    this->message_01_trigger_bang();
    this->counter_01_input_bang();
}

template<typename LISTTYPE> void inport_01_out_list_set(const LISTTYPE& v) {
    RNBO_UNUSED(v);
    this->message_01_trigger_bang();
    this->counter_01_input_bang();
}

template<typename LISTTYPE> void message_01_set_set(const LISTTYPE& v) {
    this->message_01_set = jsCreateListCopy(v);
}

void groove_01_perform(
    number rate_auto,
    number begin,
    number end,
    SampleValue * out1,
    SampleValue * sync,
    Index n
) {
    RNBO_UNUSED(out1);
    RNBO_UNUSED(end);
    RNBO_UNUSED(begin);
    RNBO_UNUSED(rate_auto);
    auto __groove_01_crossfade = this->groove_01_crossfade;
    auto __groove_01_loop = this->groove_01_loop;
    auto __groove_01_playStatus = this->groove_01_playStatus;
    auto __groove_01_readIndex = this->groove_01_readIndex;
    auto __groove_01_jumpto = this->groove_01_jumpto;
    auto __groove_01_incomingChange = this->groove_01_incomingChange;
    auto __groove_01_changeIncomingInSamples = this->groove_01_changeIncomingInSamples;
    auto __groove_01_buffer = this->groove_01_buffer;
    SampleArray<1> out = {out1};
    SampleIndex bufferLength = (SampleIndex)(__groove_01_buffer->getSize());
    Index i = 0;

    if (bufferLength > 1) {
        auto effectiveChannels = this->minimum(__groove_01_buffer->getChannels(), 1);
        number srMult = 0.001 * __groove_01_buffer->getSampleRate();
        number srInv = (number)1 / this->sr;
        number rateMult = __groove_01_buffer->getSampleRate() * srInv;

        for (; i < n; i++) {
            Index channel = 0;
            number offset = 0;
            number loopMin = 0 * srMult;
            loopMin = (loopMin > bufferLength - 1 ? bufferLength - 1 : (loopMin < 0 ? 0 : loopMin));
            number loopMax = bufferLength;
            loopMax = (loopMax > bufferLength ? bufferLength : (loopMax < 0 ? 0 : loopMax));

            if (loopMin >= loopMax) {
                offset = loopMax;
                loopMax = bufferLength;
                loopMin -= offset;
            }

            number loopLength = loopMax - loopMin;
            number currentRate = 1 * rateMult;
            number currentSync = 0;

            if (__groove_01_changeIncomingInSamples > 0) {
                __groove_01_changeIncomingInSamples--;

                if (__groove_01_changeIncomingInSamples <= 0) {
                    if (__groove_01_incomingChange == 1) {
                        if (__groove_01_jumpto >= 0) {
                            __groove_01_readIndex = __groove_01_jumpto * srMult;

                            if (__groove_01_readIndex < loopMin)
                                __groove_01_readIndex = loopMin;
                            else if (__groove_01_readIndex >= loopMax)
                                __groove_01_readIndex = loopMax - 1;

                            __groove_01_jumpto = -1;
                        } else if (currentRate < 0) {
                            __groove_01_readIndex = loopMax - 1;
                        } else {
                            __groove_01_readIndex = loopMin;
                        }

                        __groove_01_playStatus = 1;
                    } else if (__groove_01_incomingChange == 0) {
                        __groove_01_playStatus = 0;
                    }

                    __groove_01_incomingChange = 2;
                }
            }

            if (loopLength > 0) {
                if (currentRate != 0) {
                    if (__groove_01_playStatus == 1) {
                        if ((bool)(__groove_01_loop)) {
                            while (__groove_01_readIndex < loopMin) {
                                __groove_01_readIndex += loopLength;
                            }

                            while (__groove_01_readIndex >= loopMax) {
                                __groove_01_readIndex -= loopLength;
                            }
                        } else if (__groove_01_readIndex >= loopMax || __groove_01_readIndex < loopMin) {
                            __groove_01_playStatus = 0;
                            break;
                        }

                        for (; channel < effectiveChannels; channel++) {
                            number outSample = (currentRate == 1 ? this->groove_01_getSample((Index)(channel), trunc(__groove_01_readIndex), offset, bufferLength) : this->groove_01_interpolatedSample(
                                (Index)(channel),
                                __groove_01_readIndex,
                                loopMax,
                                loopLength,
                                offset,
                                bufferLength
                            ));

                            if (__groove_01_crossfade > 0) {
                                out[(Index)channel][(Index)i] = this->groove_01_crossfadedSample(
                                    outSample,
                                    __groove_01_readIndex,
                                    (Index)(channel),
                                    currentRate,
                                    loopMin,
                                    loopMax,
                                    loopLength,
                                    offset,
                                    bufferLength
                                );
                            } else {
                                out[(Index)channel][(Index)i] = outSample;
                            }
                        }

                        __groove_01_readIndex += currentRate;
                    }
                }
            }

            for (; channel < 1; channel++) {
                if (__groove_01_playStatus <= 0)
                    sync[(Index)i] = 0;

                out[(Index)channel][(Index)i] = 0;
            }
        }
    }

    for (; i < n; i++) {
        if (__groove_01_playStatus <= 0)
            sync[(Index)i] = 0;

        for (number channel = 0; channel < 1; channel++) {
            out[(Index)channel][(Index)i] = 0;
        }
    }

    this->groove_01_changeIncomingInSamples = __groove_01_changeIncomingInSamples;
    this->groove_01_incomingChange = __groove_01_incomingChange;
    this->groove_01_jumpto = __groove_01_jumpto;
    this->groove_01_readIndex = __groove_01_readIndex;
    this->groove_01_playStatus = __groove_01_playStatus;
}

void groove_02_perform(
    number rate_auto,
    number begin,
    number end,
    SampleValue * out1,
    SampleValue * sync,
    Index n
) {
    RNBO_UNUSED(out1);
    RNBO_UNUSED(end);
    RNBO_UNUSED(begin);
    RNBO_UNUSED(rate_auto);
    auto __groove_02_crossfade = this->groove_02_crossfade;
    auto __groove_02_loop = this->groove_02_loop;
    auto __groove_02_playStatus = this->groove_02_playStatus;
    auto __groove_02_readIndex = this->groove_02_readIndex;
    auto __groove_02_jumpto = this->groove_02_jumpto;
    auto __groove_02_incomingChange = this->groove_02_incomingChange;
    auto __groove_02_changeIncomingInSamples = this->groove_02_changeIncomingInSamples;
    auto __groove_02_buffer = this->groove_02_buffer;
    SampleArray<1> out = {out1};
    SampleIndex bufferLength = (SampleIndex)(__groove_02_buffer->getSize());
    Index i = 0;

    if (bufferLength > 1) {
        auto effectiveChannels = this->minimum(__groove_02_buffer->getChannels(), 1);
        number srMult = 0.001 * __groove_02_buffer->getSampleRate();
        number srInv = (number)1 / this->sr;
        number rateMult = __groove_02_buffer->getSampleRate() * srInv;

        for (; i < n; i++) {
            Index channel = 0;
            number offset = 0;
            number loopMin = 0 * srMult;
            loopMin = (loopMin > bufferLength - 1 ? bufferLength - 1 : (loopMin < 0 ? 0 : loopMin));
            number loopMax = bufferLength;
            loopMax = (loopMax > bufferLength ? bufferLength : (loopMax < 0 ? 0 : loopMax));

            if (loopMin >= loopMax) {
                offset = loopMax;
                loopMax = bufferLength;
                loopMin -= offset;
            }

            number loopLength = loopMax - loopMin;
            number currentRate = 1 * rateMult;
            number currentSync = 0;

            if (__groove_02_changeIncomingInSamples > 0) {
                __groove_02_changeIncomingInSamples--;

                if (__groove_02_changeIncomingInSamples <= 0) {
                    if (__groove_02_incomingChange == 1) {
                        if (__groove_02_jumpto >= 0) {
                            __groove_02_readIndex = __groove_02_jumpto * srMult;

                            if (__groove_02_readIndex < loopMin)
                                __groove_02_readIndex = loopMin;
                            else if (__groove_02_readIndex >= loopMax)
                                __groove_02_readIndex = loopMax - 1;

                            __groove_02_jumpto = -1;
                        } else if (currentRate < 0) {
                            __groove_02_readIndex = loopMax - 1;
                        } else {
                            __groove_02_readIndex = loopMin;
                        }

                        __groove_02_playStatus = 1;
                    } else if (__groove_02_incomingChange == 0) {
                        __groove_02_playStatus = 0;
                    }

                    __groove_02_incomingChange = 2;
                }
            }

            if (loopLength > 0) {
                if (currentRate != 0) {
                    if (__groove_02_playStatus == 1) {
                        if ((bool)(__groove_02_loop)) {
                            while (__groove_02_readIndex < loopMin) {
                                __groove_02_readIndex += loopLength;
                            }

                            while (__groove_02_readIndex >= loopMax) {
                                __groove_02_readIndex -= loopLength;
                            }
                        } else if (__groove_02_readIndex >= loopMax || __groove_02_readIndex < loopMin) {
                            __groove_02_playStatus = 0;
                            break;
                        }

                        for (; channel < effectiveChannels; channel++) {
                            number outSample = (currentRate == 1 ? this->groove_02_getSample((Index)(channel), trunc(__groove_02_readIndex), offset, bufferLength) : this->groove_02_interpolatedSample(
                                (Index)(channel),
                                __groove_02_readIndex,
                                loopMax,
                                loopLength,
                                offset,
                                bufferLength
                            ));

                            if (__groove_02_crossfade > 0) {
                                out[(Index)channel][(Index)i] = this->groove_02_crossfadedSample(
                                    outSample,
                                    __groove_02_readIndex,
                                    (Index)(channel),
                                    currentRate,
                                    loopMin,
                                    loopMax,
                                    loopLength,
                                    offset,
                                    bufferLength
                                );
                            } else {
                                out[(Index)channel][(Index)i] = outSample;
                            }
                        }

                        __groove_02_readIndex += currentRate;
                    }
                }
            }

            for (; channel < 1; channel++) {
                if (__groove_02_playStatus <= 0)
                    sync[(Index)i] = 0;

                out[(Index)channel][(Index)i] = 0;
            }
        }
    }

    for (; i < n; i++) {
        if (__groove_02_playStatus <= 0)
            sync[(Index)i] = 0;

        for (number channel = 0; channel < 1; channel++) {
            out[(Index)channel][(Index)i] = 0;
        }
    }

    this->groove_02_changeIncomingInSamples = __groove_02_changeIncomingInSamples;
    this->groove_02_incomingChange = __groove_02_incomingChange;
    this->groove_02_jumpto = __groove_02_jumpto;
    this->groove_02_readIndex = __groove_02_readIndex;
    this->groove_02_playStatus = __groove_02_playStatus;
}

void groove_03_perform(
    number rate_auto,
    number begin,
    number end,
    SampleValue * out1,
    SampleValue * sync,
    Index n
) {
    RNBO_UNUSED(out1);
    RNBO_UNUSED(end);
    RNBO_UNUSED(begin);
    RNBO_UNUSED(rate_auto);
    auto __groove_03_crossfade = this->groove_03_crossfade;
    auto __groove_03_loop = this->groove_03_loop;
    auto __groove_03_playStatus = this->groove_03_playStatus;
    auto __groove_03_readIndex = this->groove_03_readIndex;
    auto __groove_03_jumpto = this->groove_03_jumpto;
    auto __groove_03_incomingChange = this->groove_03_incomingChange;
    auto __groove_03_changeIncomingInSamples = this->groove_03_changeIncomingInSamples;
    auto __groove_03_buffer = this->groove_03_buffer;
    SampleArray<1> out = {out1};
    SampleIndex bufferLength = (SampleIndex)(__groove_03_buffer->getSize());
    Index i = 0;

    if (bufferLength > 1) {
        auto effectiveChannels = this->minimum(__groove_03_buffer->getChannels(), 1);
        number srMult = 0.001 * __groove_03_buffer->getSampleRate();
        number srInv = (number)1 / this->sr;
        number rateMult = __groove_03_buffer->getSampleRate() * srInv;

        for (; i < n; i++) {
            Index channel = 0;
            number offset = 0;
            number loopMin = 0 * srMult;
            loopMin = (loopMin > bufferLength - 1 ? bufferLength - 1 : (loopMin < 0 ? 0 : loopMin));
            number loopMax = bufferLength;
            loopMax = (loopMax > bufferLength ? bufferLength : (loopMax < 0 ? 0 : loopMax));

            if (loopMin >= loopMax) {
                offset = loopMax;
                loopMax = bufferLength;
                loopMin -= offset;
            }

            number loopLength = loopMax - loopMin;
            number currentRate = 1 * rateMult;
            number currentSync = 0;

            if (__groove_03_changeIncomingInSamples > 0) {
                __groove_03_changeIncomingInSamples--;

                if (__groove_03_changeIncomingInSamples <= 0) {
                    if (__groove_03_incomingChange == 1) {
                        if (__groove_03_jumpto >= 0) {
                            __groove_03_readIndex = __groove_03_jumpto * srMult;

                            if (__groove_03_readIndex < loopMin)
                                __groove_03_readIndex = loopMin;
                            else if (__groove_03_readIndex >= loopMax)
                                __groove_03_readIndex = loopMax - 1;

                            __groove_03_jumpto = -1;
                        } else if (currentRate < 0) {
                            __groove_03_readIndex = loopMax - 1;
                        } else {
                            __groove_03_readIndex = loopMin;
                        }

                        __groove_03_playStatus = 1;
                    } else if (__groove_03_incomingChange == 0) {
                        __groove_03_playStatus = 0;
                    }

                    __groove_03_incomingChange = 2;
                }
            }

            if (loopLength > 0) {
                if (currentRate != 0) {
                    if (__groove_03_playStatus == 1) {
                        if ((bool)(__groove_03_loop)) {
                            while (__groove_03_readIndex < loopMin) {
                                __groove_03_readIndex += loopLength;
                            }

                            while (__groove_03_readIndex >= loopMax) {
                                __groove_03_readIndex -= loopLength;
                            }
                        } else if (__groove_03_readIndex >= loopMax || __groove_03_readIndex < loopMin) {
                            __groove_03_playStatus = 0;
                            break;
                        }

                        for (; channel < effectiveChannels; channel++) {
                            number outSample = (currentRate == 1 ? this->groove_03_getSample((Index)(channel), trunc(__groove_03_readIndex), offset, bufferLength) : this->groove_03_interpolatedSample(
                                (Index)(channel),
                                __groove_03_readIndex,
                                loopMax,
                                loopLength,
                                offset,
                                bufferLength
                            ));

                            if (__groove_03_crossfade > 0) {
                                out[(Index)channel][(Index)i] = this->groove_03_crossfadedSample(
                                    outSample,
                                    __groove_03_readIndex,
                                    (Index)(channel),
                                    currentRate,
                                    loopMin,
                                    loopMax,
                                    loopLength,
                                    offset,
                                    bufferLength
                                );
                            } else {
                                out[(Index)channel][(Index)i] = outSample;
                            }
                        }

                        __groove_03_readIndex += currentRate;
                    }
                }
            }

            for (; channel < 1; channel++) {
                if (__groove_03_playStatus <= 0)
                    sync[(Index)i] = 0;

                out[(Index)channel][(Index)i] = 0;
            }
        }
    }

    for (; i < n; i++) {
        if (__groove_03_playStatus <= 0)
            sync[(Index)i] = 0;

        for (number channel = 0; channel < 1; channel++) {
            out[(Index)channel][(Index)i] = 0;
        }
    }

    this->groove_03_changeIncomingInSamples = __groove_03_changeIncomingInSamples;
    this->groove_03_incomingChange = __groove_03_incomingChange;
    this->groove_03_jumpto = __groove_03_jumpto;
    this->groove_03_readIndex = __groove_03_readIndex;
    this->groove_03_playStatus = __groove_03_playStatus;
}

void signaladder_01_perform(
    const SampleValue * in1,
    const SampleValue * in2,
    const SampleValue * in3,
    SampleValue * out,
    Index n
) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out[(Index)i] = in1[(Index)i] + in2[(Index)i] + in3[(Index)i];
    }
}

void signaladder_02_perform(
    const SampleValue * in1,
    const SampleValue * in2,
    const SampleValue * in3,
    SampleValue * out,
    Index n
) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out[(Index)i] = in1[(Index)i] + in2[(Index)i] + in3[(Index)i];
    }
}

void stackprotect_perform(Index n) {
    RNBO_UNUSED(n);
    auto __stackprotect_count = this->stackprotect_count;
    __stackprotect_count = 0;
    this->stackprotect_count = __stackprotect_count;
}

void data_01_srout_set(number ) {}

void data_01_chanout_set(number ) {}

void data_01_sizeout_set(number v) {
    this->data_01_sizeout = v;
}

void data_02_srout_set(number ) {}

void data_02_chanout_set(number ) {}

void data_02_sizeout_set(number v) {
    this->data_02_sizeout = v;
}

void data_03_srout_set(number ) {}

void data_03_chanout_set(number ) {}

void data_03_sizeout_set(number v) {
    this->data_03_sizeout = v;
}

number groove_01_getSample(
    Index channel,
    SampleIndex index,
    SampleIndex offset,
    SampleIndex bufferLength
) {
    if (offset > 0) {
        index += offset;

        if (index >= bufferLength)
            index -= bufferLength;
    }

    return this->groove_01_buffer->getSample(channel, index);
}

number groove_01_interpolatedSample(
    Index channel,
    number index,
    SampleIndex end,
    SampleIndex length,
    SampleIndex offset,
    SampleIndex bufferLength
) {
    SampleIndex index1 = (SampleIndex)(trunc(index));
    number i_x = index - index1;
    number i_1px = 1. + i_x;
    number i_1mx = 1. - i_x;
    number i_2mx = 2. - i_x;
    number i_a = i_1mx * i_2mx;
    number i_b = i_1px * i_x;
    number i_p1 = -.1666667 * i_a * i_x;
    number i_p2 = .5 * i_1px * i_a;
    number i_p3 = .5 * i_b * i_2mx;
    number i_p4 = -.1666667 * i_b * i_1mx;
    SampleIndex index2 = (SampleIndex)(index1 + 1);

    if (index2 >= end)
        index2 -= length;

    SampleIndex index3 = (SampleIndex)(index1 + 2);

    if (index3 >= end)
        index3 -= length;

    SampleIndex index4 = (SampleIndex)(index1 + 3);

    if (index4 >= end)
        index4 -= length;

    return this->groove_01_getSample(channel, index1, offset, bufferLength) * i_p1 + this->groove_01_getSample(channel, index2, offset, bufferLength) * i_p2 + this->groove_01_getSample(channel, index3, offset, bufferLength) * i_p3 + this->groove_01_getSample(channel, index4, offset, bufferLength) * i_p4;
}

number groove_01_crossfadedSample(
    SampleValue out,
    number readIndex,
    Index channel,
    number rate,
    number loopMin,
    number loopMax,
    number loopLength,
    number offset,
    number bufferLength
) {
    auto crossFadeStart1 = this->maximum(loopMin - this->groove_01_crossfadeInSamples, 0);
    auto crossFadeEnd1 = this->minimum(crossFadeStart1 + this->groove_01_crossfadeInSamples, bufferLength);
    number crossFadeStart2 = crossFadeStart1 + loopLength;
    auto crossFadeEnd2 = this->minimum(crossFadeEnd1 + loopLength, bufferLength);
    number crossFadeLength = crossFadeEnd2 - crossFadeStart2;

    if (crossFadeLength > 0) {
        crossFadeEnd1 = crossFadeStart1 + crossFadeLength;
        number diff = -1;
        number addFactor = 0;

        if (readIndex >= crossFadeStart2) {
            diff = readIndex - crossFadeStart2;
            addFactor = -1;
        } else if (readIndex < crossFadeEnd1) {
            diff = crossFadeEnd1 - readIndex + loopMax - crossFadeStart2;
            addFactor = 1;
        }

        if (diff >= 0) {
            number out2ReadIndex = readIndex + loopLength * addFactor;
            number out2 = (rate == 1 ? this->groove_01_getSample(channel, trunc(out2ReadIndex), offset, bufferLength) : this->groove_01_interpolatedSample(channel, out2ReadIndex, loopMax, loopLength, offset, bufferLength));
            number out2Factor = diff / crossFadeLength;
            number out1Factor = 1 - out2Factor;
            return out * out1Factor + out2 * out2Factor;
        }
    }

    return out;
}

void groove_01_dspsetup(bool force) {
    if ((bool)(this->groove_01_setupDone) && (bool)(!(bool)(force)))
        return;

    this->groove_01_crossfadeInSamples = this->mstosamps(this->groove_01_crossfade);
    this->groove_01_setupDone = true;
}

void counter_01_inc() {
    this->counter_01_count = ((bool)(this->counter_01_phase) ? this->counter_01_count + 1 : this->counter_01_count - 1);
}

number groove_02_getSample(
    Index channel,
    SampleIndex index,
    SampleIndex offset,
    SampleIndex bufferLength
) {
    if (offset > 0) {
        index += offset;

        if (index >= bufferLength)
            index -= bufferLength;
    }

    return this->groove_02_buffer->getSample(channel, index);
}

number groove_02_interpolatedSample(
    Index channel,
    number index,
    SampleIndex end,
    SampleIndex length,
    SampleIndex offset,
    SampleIndex bufferLength
) {
    SampleIndex index1 = (SampleIndex)(trunc(index));
    number i_x = index - index1;
    number i_1px = 1. + i_x;
    number i_1mx = 1. - i_x;
    number i_2mx = 2. - i_x;
    number i_a = i_1mx * i_2mx;
    number i_b = i_1px * i_x;
    number i_p1 = -.1666667 * i_a * i_x;
    number i_p2 = .5 * i_1px * i_a;
    number i_p3 = .5 * i_b * i_2mx;
    number i_p4 = -.1666667 * i_b * i_1mx;
    SampleIndex index2 = (SampleIndex)(index1 + 1);

    if (index2 >= end)
        index2 -= length;

    SampleIndex index3 = (SampleIndex)(index1 + 2);

    if (index3 >= end)
        index3 -= length;

    SampleIndex index4 = (SampleIndex)(index1 + 3);

    if (index4 >= end)
        index4 -= length;

    return this->groove_02_getSample(channel, index1, offset, bufferLength) * i_p1 + this->groove_02_getSample(channel, index2, offset, bufferLength) * i_p2 + this->groove_02_getSample(channel, index3, offset, bufferLength) * i_p3 + this->groove_02_getSample(channel, index4, offset, bufferLength) * i_p4;
}

number groove_02_crossfadedSample(
    SampleValue out,
    number readIndex,
    Index channel,
    number rate,
    number loopMin,
    number loopMax,
    number loopLength,
    number offset,
    number bufferLength
) {
    auto crossFadeStart1 = this->maximum(loopMin - this->groove_02_crossfadeInSamples, 0);
    auto crossFadeEnd1 = this->minimum(crossFadeStart1 + this->groove_02_crossfadeInSamples, bufferLength);
    number crossFadeStart2 = crossFadeStart1 + loopLength;
    auto crossFadeEnd2 = this->minimum(crossFadeEnd1 + loopLength, bufferLength);
    number crossFadeLength = crossFadeEnd2 - crossFadeStart2;

    if (crossFadeLength > 0) {
        crossFadeEnd1 = crossFadeStart1 + crossFadeLength;
        number diff = -1;
        number addFactor = 0;

        if (readIndex >= crossFadeStart2) {
            diff = readIndex - crossFadeStart2;
            addFactor = -1;
        } else if (readIndex < crossFadeEnd1) {
            diff = crossFadeEnd1 - readIndex + loopMax - crossFadeStart2;
            addFactor = 1;
        }

        if (diff >= 0) {
            number out2ReadIndex = readIndex + loopLength * addFactor;
            number out2 = (rate == 1 ? this->groove_02_getSample(channel, trunc(out2ReadIndex), offset, bufferLength) : this->groove_02_interpolatedSample(channel, out2ReadIndex, loopMax, loopLength, offset, bufferLength));
            number out2Factor = diff / crossFadeLength;
            number out1Factor = 1 - out2Factor;
            return out * out1Factor + out2 * out2Factor;
        }
    }

    return out;
}

void groove_02_dspsetup(bool force) {
    if ((bool)(this->groove_02_setupDone) && (bool)(!(bool)(force)))
        return;

    this->groove_02_crossfadeInSamples = this->mstosamps(this->groove_02_crossfade);
    this->groove_02_setupDone = true;
}

number groove_03_getSample(
    Index channel,
    SampleIndex index,
    SampleIndex offset,
    SampleIndex bufferLength
) {
    if (offset > 0) {
        index += offset;

        if (index >= bufferLength)
            index -= bufferLength;
    }

    return this->groove_03_buffer->getSample(channel, index);
}

number groove_03_interpolatedSample(
    Index channel,
    number index,
    SampleIndex end,
    SampleIndex length,
    SampleIndex offset,
    SampleIndex bufferLength
) {
    SampleIndex index1 = (SampleIndex)(trunc(index));
    number i_x = index - index1;
    number i_1px = 1. + i_x;
    number i_1mx = 1. - i_x;
    number i_2mx = 2. - i_x;
    number i_a = i_1mx * i_2mx;
    number i_b = i_1px * i_x;
    number i_p1 = -.1666667 * i_a * i_x;
    number i_p2 = .5 * i_1px * i_a;
    number i_p3 = .5 * i_b * i_2mx;
    number i_p4 = -.1666667 * i_b * i_1mx;
    SampleIndex index2 = (SampleIndex)(index1 + 1);

    if (index2 >= end)
        index2 -= length;

    SampleIndex index3 = (SampleIndex)(index1 + 2);

    if (index3 >= end)
        index3 -= length;

    SampleIndex index4 = (SampleIndex)(index1 + 3);

    if (index4 >= end)
        index4 -= length;

    return this->groove_03_getSample(channel, index1, offset, bufferLength) * i_p1 + this->groove_03_getSample(channel, index2, offset, bufferLength) * i_p2 + this->groove_03_getSample(channel, index3, offset, bufferLength) * i_p3 + this->groove_03_getSample(channel, index4, offset, bufferLength) * i_p4;
}

number groove_03_crossfadedSample(
    SampleValue out,
    number readIndex,
    Index channel,
    number rate,
    number loopMin,
    number loopMax,
    number loopLength,
    number offset,
    number bufferLength
) {
    auto crossFadeStart1 = this->maximum(loopMin - this->groove_03_crossfadeInSamples, 0);
    auto crossFadeEnd1 = this->minimum(crossFadeStart1 + this->groove_03_crossfadeInSamples, bufferLength);
    number crossFadeStart2 = crossFadeStart1 + loopLength;
    auto crossFadeEnd2 = this->minimum(crossFadeEnd1 + loopLength, bufferLength);
    number crossFadeLength = crossFadeEnd2 - crossFadeStart2;

    if (crossFadeLength > 0) {
        crossFadeEnd1 = crossFadeStart1 + crossFadeLength;
        number diff = -1;
        number addFactor = 0;

        if (readIndex >= crossFadeStart2) {
            diff = readIndex - crossFadeStart2;
            addFactor = -1;
        } else if (readIndex < crossFadeEnd1) {
            diff = crossFadeEnd1 - readIndex + loopMax - crossFadeStart2;
            addFactor = 1;
        }

        if (diff >= 0) {
            number out2ReadIndex = readIndex + loopLength * addFactor;
            number out2 = (rate == 1 ? this->groove_03_getSample(channel, trunc(out2ReadIndex), offset, bufferLength) : this->groove_03_interpolatedSample(channel, out2ReadIndex, loopMax, loopLength, offset, bufferLength));
            number out2Factor = diff / crossFadeLength;
            number out1Factor = 1 - out2Factor;
            return out * out1Factor + out2 * out2Factor;
        }
    }

    return out;
}

void groove_03_dspsetup(bool force) {
    if ((bool)(this->groove_03_setupDone) && (bool)(!(bool)(force)))
        return;

    this->groove_03_crossfadeInSamples = this->mstosamps(this->groove_03_crossfade);
    this->groove_03_setupDone = true;
}

void message_01_init() {
    this->message_01_set_set(listbase<number, 1>{5});
}

void data_01_init() {
    this->data_01_buffer->setWantsFill(true);
}

Index data_01_evaluateSizeExpr(number samplerate, number vectorsize) {
    RNBO_UNUSED(vectorsize);
    RNBO_UNUSED(samplerate);
    number size = 0;
    return (Index)(size);
}

void data_01_dspsetup(bool force) {
    if ((bool)(this->data_01_setupDone) && (bool)(!(bool)(force)))
        return;

    if (this->data_01_sizemode == 2) {
        this->data_01_buffer = this->data_01_buffer->setSize((Index)(this->mstosamps(this->data_01_sizems)));
        updateDataRef(this, this->data_01_buffer);
    } else if (this->data_01_sizemode == 3) {
        this->data_01_buffer = this->data_01_buffer->setSize(this->data_01_evaluateSizeExpr(this->sr, this->vs));
        updateDataRef(this, this->data_01_buffer);
    }

    this->data_01_setupDone = true;
}

void data_01_bufferUpdated() {
    this->data_01_report();
}

void data_01_report() {
    this->data_01_srout_set(this->data_01_buffer->getSampleRate());
    this->data_01_chanout_set(this->data_01_buffer->getChannels());
    this->data_01_sizeout_set(this->data_01_buffer->getSize());
}

void data_02_init() {
    this->data_02_buffer->setWantsFill(true);
}

Index data_02_evaluateSizeExpr(number samplerate, number vectorsize) {
    RNBO_UNUSED(vectorsize);
    RNBO_UNUSED(samplerate);
    number size = 0;
    return (Index)(size);
}

void data_02_dspsetup(bool force) {
    if ((bool)(this->data_02_setupDone) && (bool)(!(bool)(force)))
        return;

    if (this->data_02_sizemode == 2) {
        this->data_02_buffer = this->data_02_buffer->setSize((Index)(this->mstosamps(this->data_02_sizems)));
        updateDataRef(this, this->data_02_buffer);
    } else if (this->data_02_sizemode == 3) {
        this->data_02_buffer = this->data_02_buffer->setSize(this->data_02_evaluateSizeExpr(this->sr, this->vs));
        updateDataRef(this, this->data_02_buffer);
    }

    this->data_02_setupDone = true;
}

void data_02_bufferUpdated() {
    this->data_02_report();
}

void data_02_report() {
    this->data_02_srout_set(this->data_02_buffer->getSampleRate());
    this->data_02_chanout_set(this->data_02_buffer->getChannels());
    this->data_02_sizeout_set(this->data_02_buffer->getSize());
}

void data_03_init() {
    this->data_03_buffer->setWantsFill(true);
}

Index data_03_evaluateSizeExpr(number samplerate, number vectorsize) {
    RNBO_UNUSED(vectorsize);
    RNBO_UNUSED(samplerate);
    number size = 0;
    return (Index)(size);
}

void data_03_dspsetup(bool force) {
    if ((bool)(this->data_03_setupDone) && (bool)(!(bool)(force)))
        return;

    if (this->data_03_sizemode == 2) {
        this->data_03_buffer = this->data_03_buffer->setSize((Index)(this->mstosamps(this->data_03_sizems)));
        updateDataRef(this, this->data_03_buffer);
    } else if (this->data_03_sizemode == 3) {
        this->data_03_buffer = this->data_03_buffer->setSize(this->data_03_evaluateSizeExpr(this->sr, this->vs));
        updateDataRef(this, this->data_03_buffer);
    }

    this->data_03_setupDone = true;
}

void data_03_bufferUpdated() {
    this->data_03_report();
}

void data_03_report() {
    this->data_03_srout_set(this->data_03_buffer->getSampleRate());
    this->data_03_chanout_set(this->data_03_buffer->getChannels());
    this->data_03_sizeout_set(this->data_03_buffer->getSize());
}

void globaltransport_advance() {}

void globaltransport_dspsetup(bool ) {}

bool stackprotect_check() {
    this->stackprotect_count++;

    if (this->stackprotect_count > 128) {
        console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
        return true;
    }

    return false;
}

Index getPatcherSerial() const {
    return 0;
}

void sendParameter(ParameterIndex index, bool ignoreValue) {
    this->getEngine()->notifyParameterValueChanged(index, (ignoreValue ? 0 : this->getParameterValue(index)), ignoreValue);
}

void scheduleParamInit(ParameterIndex index, Index order) {
    this->paramInitIndices->push(index);
    this->paramInitOrder->push(order);
}

void processParamInitEvents() {
    this->listquicksort(
        this->paramInitOrder,
        this->paramInitIndices,
        0,
        (int)(this->paramInitOrder->length - 1),
        true
    );

    for (Index i = 0; i < this->paramInitOrder->length; i++) {
        this->getEngine()->scheduleParameterBang(this->paramInitIndices[i], 0);
    }
}

void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
    RNBO_UNUSED(inProcess);
    RNBO_UNUSED(engine);
    this->_currentTime = time;
    auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));

    if (offset >= (SampleIndex)(this->vs))
        offset = (SampleIndex)(this->vs) - 1;

    if (offset < 0)
        offset = 0;

    this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
}

void assign_defaults()
{
    groove_01_rate_auto = 1;
    groove_01_jumpto = -1;
    groove_01_begin = 0;
    groove_01_end = -1;
    groove_01_loop = 0;
    groove_01_crossfade = 0;
    route_01_selector1 = 0;
    route_01_selector2 = 1;
    route_01_selector3 = 2;
    counter_01_carryflag = 0;
    counter_01_direction = 0;
    counter_01_resetnext_number = 0;
    counter_01_resetnow_number = 0;
    counter_01_maximum = 2;
    counter_01_output = 0;
    counter_01_underflow_number = 0;
    counter_01_overflow_number = 0;
    counter_01_carry = 0;
    counter_01_minimum = 0;
    groove_02_rate_auto = 1;
    groove_02_jumpto = -1;
    groove_02_begin = 0;
    groove_02_end = -1;
    groove_02_loop = 0;
    groove_02_crossfade = 0;
    groove_03_rate_auto = 1;
    groove_03_jumpto = -1;
    groove_03_begin = 0;
    groove_03_end = -1;
    groove_03_loop = 0;
    groove_03_crossfade = 0;
    data_01_sizeout = 0;
    data_01_size = 0;
    data_01_sizems = 0;
    data_01_normalize = 0.995;
    data_01_channels = 1;
    data_02_sizeout = 0;
    data_02_size = 0;
    data_02_sizems = 0;
    data_02_normalize = 0.995;
    data_02_channels = 1;
    data_03_sizeout = 0;
    data_03_size = 0;
    data_03_sizems = 0;
    data_03_normalize = 0.995;
    data_03_channels = 1;
    _currentTime = 0;
    audioProcessSampleCount = 0;
    sampleOffsetIntoNextAudioBuffer = 0;
    zeroBuffer = nullptr;
    dummyBuffer = nullptr;
    signals[0] = nullptr;
    signals[1] = nullptr;
    signals[2] = nullptr;
    didAllocateSignals = 0;
    vs = 0;
    maxvs = 0;
    sr = 44100;
    invsr = 0.000022675736961451248;
    groove_01_readIndex = 0;
    groove_01_playStatus = 0;
    groove_01_changeIncomingInSamples = 0;
    groove_01_incomingChange = 2;
    groove_01_crossfadeInSamples = 0;
    groove_01_setupDone = false;
    counter_01_count = 0;
    counter_01_phase = true;
    groove_02_readIndex = 0;
    groove_02_playStatus = 0;
    groove_02_changeIncomingInSamples = 0;
    groove_02_incomingChange = 2;
    groove_02_crossfadeInSamples = 0;
    groove_02_setupDone = false;
    groove_03_readIndex = 0;
    groove_03_playStatus = 0;
    groove_03_changeIncomingInSamples = 0;
    groove_03_incomingChange = 2;
    groove_03_crossfadeInSamples = 0;
    groove_03_setupDone = false;
    data_01_sizemode = 0;
    data_01_setupDone = false;
    data_02_sizemode = 0;
    data_02_setupDone = false;
    data_03_sizemode = 0;
    data_03_setupDone = false;
    globaltransport_tempo = nullptr;
    globaltransport_state = nullptr;
    stackprotect_count = 0;
    _voiceIndex = 0;
    _noteNumber = 0;
    isMuted = 1;
}

    // data ref strings
    struct DataRefStrings {
    	static constexpr auto& name0 = "bouclier00";
    	static constexpr auto& file0 = "boubou01.wav";
    	static constexpr auto& tag0 = "buffer~";
    	static constexpr auto& name1 = "bouclier01";
    	static constexpr auto& file1 = "boubou02.wav";
    	static constexpr auto& tag1 = "buffer~";
    	static constexpr auto& name2 = "bouclier02";
    	static constexpr auto& file2 = "boubou03.wav";
    	static constexpr auto& tag2 = "buffer~";
    	DataRefStrings* operator->() { return this; }
    	const DataRefStrings* operator->() const { return this; }
    };

    DataRefStrings dataRefStrings;

// member variables

    number groove_01_rate_auto;
    number groove_01_jumpto;
    number groove_01_begin;
    number groove_01_end;
    number groove_01_loop;
    number groove_01_crossfade;
    number route_01_selector1;
    number route_01_selector2;
    number route_01_selector3;
    Int counter_01_carryflag;
    number counter_01_direction;
    number counter_01_resetnext_number;
    number counter_01_resetnow_number;
    number counter_01_maximum;
    number counter_01_output;
    number counter_01_underflow_number;
    number counter_01_overflow_number;
    number counter_01_carry;
    number counter_01_minimum;
    number groove_02_rate_auto;
    number groove_02_jumpto;
    number groove_02_begin;
    number groove_02_end;
    number groove_02_loop;
    number groove_02_crossfade;
    number groove_03_rate_auto;
    number groove_03_jumpto;
    number groove_03_begin;
    number groove_03_end;
    number groove_03_loop;
    number groove_03_crossfade;
    list message_01_set;
    number data_01_sizeout;
    number data_01_size;
    number data_01_sizems;
    number data_01_normalize;
    number data_01_channels;
    number data_02_sizeout;
    number data_02_size;
    number data_02_sizems;
    number data_02_normalize;
    number data_02_channels;
    number data_03_sizeout;
    number data_03_size;
    number data_03_sizems;
    number data_03_normalize;
    number data_03_channels;
    MillisecondTime _currentTime;
    ENGINE _internalEngine;
    UInt64 audioProcessSampleCount;
    Index sampleOffsetIntoNextAudioBuffer;
    signal zeroBuffer;
    signal dummyBuffer;
    SampleValue * signals[3];
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    Float32BufferRef groove_01_buffer;
    number groove_01_readIndex;
    Index groove_01_playStatus;
    SampleIndex groove_01_changeIncomingInSamples;
    Int groove_01_incomingChange;
    SampleIndex groove_01_crossfadeInSamples;
    bool groove_01_setupDone;
    number counter_01_count;
    bool counter_01_phase;
    Float32BufferRef groove_02_buffer;
    number groove_02_readIndex;
    Index groove_02_playStatus;
    SampleIndex groove_02_changeIncomingInSamples;
    Int groove_02_incomingChange;
    SampleIndex groove_02_crossfadeInSamples;
    bool groove_02_setupDone;
    Float32BufferRef groove_03_buffer;
    number groove_03_readIndex;
    Index groove_03_playStatus;
    SampleIndex groove_03_changeIncomingInSamples;
    Int groove_03_incomingChange;
    SampleIndex groove_03_crossfadeInSamples;
    bool groove_03_setupDone;
    Float32BufferRef data_01_buffer;
    Int data_01_sizemode;
    bool data_01_setupDone;
    Float32BufferRef data_02_buffer;
    Int data_02_sizemode;
    bool data_02_setupDone;
    Float32BufferRef data_03_buffer;
    Int data_03_sizemode;
    bool data_03_setupDone;
    signal globaltransport_tempo;
    signal globaltransport_state;
    number stackprotect_count;
    DataRef bouclier00;
    DataRef bouclier01;
    DataRef bouclier02;
    Index _voiceIndex;
    Int _noteNumber;
    Index isMuted;
    indexlist paramInitIndices;
    indexlist paramInitOrder;
    bool _isInitialized = false;
};

static PatcherInterface* creaternbomatic()
{
    return new rnbomatic<EXTERNALENGINE>();
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" PatcherFactoryFunctionPtr GetPatcherFactoryFunction()
#else
extern "C" PatcherFactoryFunctionPtr rnbomaticFactoryFunction()
#endif
{
    return creaternbomatic;
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" void SetLogger(Logger* logger)
#else
void rnbomaticSetLogger(Logger* logger)
#endif
{
    console = logger;
}

} // end RNBO namespace

