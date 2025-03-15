#include "WirelessSync.h"

// Static instance pointer for callback
static WirelessSync* wirelessSyncInstance = nullptr;

// Static callback function for ESP-NOW
void WirelessSync::onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(SyncMessage)) {
    Serial.println("Invalid message size");
    return;
  }
  
  // Convert data to SyncMessage
  SyncMessage *msg = (SyncMessage*)data;
  
  // Skip our own messages
  if (wirelessSyncInstance && memcmp(msg->deviceID, wirelessSyncInstance->_deviceID, 6) == 0) {
    return;
  }
  
  // Update latency tracking
  if (wirelessSyncInstance) {
    wirelessSyncInstance->updateLatency(msg->timestamp);
  }
  
  // Process message based on type
  switch (msg->type) {
    case MSG_CLOCK:
      // Update leader heartbeat time
      if (msg->data.clock.isLeader) {
        if (wirelessSyncInstance) {
          memcpy(wirelessSyncInstance->_currentLeaderID, msg->deviceID, 6);
          wirelessSyncInstance->_lastLeaderHeartbeat = millis();
          
          // Enhanced sync prediction
          wirelessSyncInstance->predictNextTick(msg->data.clock.clockTick, msg->timestamp);
          
          // Apply drift correction to uClock
          float correctedTempo = uClock.getTempo() * wirelessSyncInstance->_driftCorrection;
          uClock.setTempo(correctedTempo);
        }
      }
      break;
      
    case MSG_BEAT:
      // Process beat message (for followers)
      // Update BPM if needed
      if (wirelessSyncInstance && !wirelessSyncInstance->_isLeader) {
        float currentBpm = uClock.getTempo();
        float newBpm = msg->data.beat.bpm;
        
        // Only update if BPM has changed significantly
        if (abs(currentBpm - newBpm) > 0.5) {
          uClock.setTempo(newBpm);
        }
      }
      break;
      
    case MSG_CONTROL:
      // Process control messages
      if (msg->data.control.command == CMD_RESET && msg->data.control.param1 == 1) {
        // This is a leader negotiation message
        if (wirelessSyncInstance) {
          wirelessSyncInstance->processLeaderSelection(*msg);
        }
      }
      break;
      
    default:
      // Other message types handled by specific implementations
      break;
  }
}

bool WirelessSync::init() {
  // Set static instance for callback at initialization
  wirelessSyncInstance = this;
  
  // Set device in station mode
  WiFi.mode(WIFI_STA);
  
  // Get MAC address
  WiFi.macAddress(_deviceID);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return false;
  }
  
  // Register callback
  esp_now_register_recv_cb(onDataReceived);
  
  // Register peer (broadcast address)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, _broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return false;
  }
  
  Serial.println("ESP-NOW initialized successfully");
  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(_deviceID[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
  return true;
}

void WirelessSync::setAsLeader(bool isLeader) {
  _isLeader = isLeader;
}

void WirelessSync::sendMessage(SyncMessage &msg) {
  // Fill common fields
  msg.sequenceNum = _sequenceNum++;
  msg.priority = _priority;
  memcpy(msg.deviceID, _deviceID, 6);
  _lastSendTime = micros(); // Track send time
  msg.timestamp = _lastSendTime; // Use actual send time
  
  // Send message
  esp_err_t result = esp_now_send(_broadcastAddress, (uint8_t *)&msg, sizeof(SyncMessage));
  
  if (result != ESP_OK) {
    Serial.println("Error sending ESP-NOW message");
  }
}

// Handler for uClock's Sync24 callback (24 PPQN)
void WirelessSync::onSync24(uint32_t tick) {
  if (_isLeader) {
    // Don't send every SYNC24 at high tempos to avoid overloading the network
    // For tempos > 120 BPM, send every other SYNC24 pulse
    // For tempos > 240 BPM, send every fourth SYNC24 pulse
    if (uClock.getTempo() <= 120 || 
        (uClock.getTempo() <= 240 && tick % 2 == 0) ||
        (tick % 4 == 0)) {
      sendClock(tick);
    }
  }
}

// Handler for uClock's PPQN callback
void WirelessSync::onPPQN(uint32_t tick, MetronomeState &state) {
  if (_isLeader) {
    // Every quarter note (96 PPQN ticks)
    if (tick % 96 == 0) {
      uint32_t quarterNote = tick / 96;
      if (quarterNote != _lastQuarterNote) {
        _lastQuarterNote = quarterNote;
        sendBeat(quarterNote, state);
      }
    }
  }
}

// Handler for uClock's Step callback
void WirelessSync::onStep(uint32_t step, MetronomeState &state) {
  if (_isLeader) {
    // This is called at the start of each "step" (usually quarter notes)
    // We'll use it to detect the start of patterns/bars
    sendBar(step, state);
  }
}

void WirelessSync::sendClock(uint32_t tick) {
  SyncMessage msg;
  msg.type = MSG_CLOCK;
  msg.data.clock.isLeader = _isLeader ? 1 : 0;
  msg.data.clock.clockTick = tick;
  memset(msg.data.clock.reserved, 0, sizeof(msg.data.clock.reserved));
  
  sendMessage(msg);
}

void WirelessSync::sendBeat(uint32_t beat, MetronomeState &state) {
  SyncMessage msg;
  msg.type = MSG_BEAT;
  msg.data.beat.bpm = uClock.getTempo();
  msg.data.beat.beatPosition = beat % 4; // Assuming 4/4 time for now
  msg.data.beat.multiplierIdx = state.currentMultiplierIndex;
  memset(msg.data.beat.reserved, 0, sizeof(msg.data.beat.reserved));
  
  sendMessage(msg);
}

void WirelessSync::sendBar(uint32_t bar, MetronomeState &state) {
  SyncMessage msg;
  msg.type = MSG_BAR;
  msg.data.bar.globalBar = bar;
  msg.data.bar.channelCount = MetronomeState::CHANNEL_COUNT;
  
  // Get pattern length from first enabled channel or use default
  uint16_t patternLength = 4; // Default pattern length
  uint8_t activePattern = 0;  // Default pattern ID
  
  // Create channel mask with enabled channels
  uint32_t channelMask = 0;
  
  // Populate channel mask and get pattern info from state
  for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++) {
    const MetronomeChannel &channel = state.getChannel(i);
    if (channel.isEnabled()) {
      // Set bit for enabled channel
      channelMask |= (1 << i);
      
      // Use first enabled channel's pattern length
      if (patternLength == 4) { // If still default
        patternLength = channel.getBarLength();
      }
    }
  }
  
  msg.data.bar.patternLength = patternLength;
  msg.data.bar.activePattern = activePattern; // TODO: Get active pattern from state if needed
  msg.data.bar.channelMask = channelMask;
  
  sendMessage(msg);
}

void WirelessSync::sendPattern(MetronomeState &state, uint8_t channelId) {
  if (channelId >= MetronomeState::CHANNEL_COUNT) return;
  
  const MetronomeChannel &channel = state.getChannel(channelId);
  
  SyncMessage msg;
  msg.type = MSG_PATTERN;
  msg.data.pattern.channelId = channelId;
  msg.data.pattern.barLength = channel.getBarLength();
  msg.data.pattern.pattern = channel.getPattern();
  msg.data.pattern.currentBeat = channel.getCurrentBeat();
  msg.data.pattern.enabled = channel.isEnabled() ? 1 : 0;
  memset(msg.data.pattern.reserved, 0, sizeof(msg.data.pattern.reserved));
  
  sendMessage(msg);
}

void WirelessSync::sendControl(uint8_t command, uint32_t value) {
  SyncMessage msg;
  msg.type = MSG_CONTROL;
  msg.data.control.command = command;
  msg.data.control.param1 = 0;
  msg.data.control.param2 = 0;
  msg.data.control.param3 = 0;
  msg.data.control.value = value;
  
  sendMessage(msg);
}

void WirelessSync::notifyPatternChanged(uint8_t channelId) {
  _patternChanged = true;
}

void WirelessSync::update(MetronomeState &state) {
  // Only used for detecting pattern changes and sending them immediately
  // Most messages are sent from uClock callbacks now
  
  if (_patternChanged) {
    _patternChanged = false;
    
    // Send updated pattern info for all channels
    for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++) {
      sendPattern(state, i);
    }
  }
}

// Set device priority (higher value = higher priority)
void WirelessSync::setPriority(uint8_t priority) {
  _priority = priority;
}

// Check if this device is currently the leader
bool WirelessSync::isLeader() const {
  return _isLeader;
}

// Start leader negotiation process
void WirelessSync::startLeaderNegotiation() {
  _leaderNegotiationActive = true;
  _highestPrioritySeen = _priority;
  memcpy(_highestPriorityDevice, _deviceID, 6);
  
  // Send control message to start negotiation
  SyncMessage msg;
  msg.type = MSG_CONTROL;
  msg.data.control.command = CMD_RESET; // Use RESET command for leader negotiation
  msg.data.control.param1 = 1; // Param1=1 means leader negotiation
  msg.data.control.param2 = 0;
  msg.data.control.param3 = 0;
  msg.data.control.value = _priority; // Send our priority
  
  sendMessage(msg);
  
  // Wait for responses (will be handled in onDataReceived)
  delay(500); // Give other devices time to respond
  
  // Check if we're the highest priority device
  if (memcmp(_highestPriorityDevice, _deviceID, 6) == 0) {
    // We're the highest priority, become leader
    _isLeader = true;
    Serial.println("This device is now the leader");
  } else {
    // Another device has higher priority
    _isLeader = false;
    Serial.println("Another device is the leader");
  }
  
  _leaderNegotiationActive = false;
}

// Process leader selection from received message
void WirelessSync::processLeaderSelection(const SyncMessage &msg) {
  // Only process during active negotiation
  if (!_leaderNegotiationActive) return;
  
  // Check if the received priority is higher than what we've seen
  if (isHigherPriority(msg.deviceID, msg.priority)) {
    _highestPrioritySeen = msg.priority;
    memcpy(_highestPriorityDevice, msg.deviceID, 6);
  }
}

// Check if a device has higher priority than current highest
bool WirelessSync::isHigherPriority(const uint8_t *deviceID, uint8_t priority) {
  if (priority > _highestPrioritySeen) {
    return true;
  } else if (priority == _highestPrioritySeen) {
    // If priorities are equal, use MAC address as tiebreaker
    return memcmp(deviceID, _highestPriorityDevice, 6) < 0;
  }
  return false;
}

// Check if leader has timed out
bool WirelessSync::isLeaderTimedOut() {
  // If we're the leader, we can't time out
  if (_isLeader) return false;
  
  // If no leader heartbeat received within timeout period
  return (millis() - _lastLeaderHeartbeat) > _leaderTimeoutMs;
}

// Public method to initiate leadership negotiation
void WirelessSync::negotiateLeadership() {
  startLeaderNegotiation();
}

// Check leader status and initiate negotiation if needed
void WirelessSync::checkLeaderStatus() {
  if (!_isLeader && isLeaderTimedOut()) {
    Serial.println("Leader timed out, starting negotiation");
    startLeaderNegotiation();
  }
}

void WirelessSync::updateLatency(uint64_t sendTime) {
  uint32_t currentLatency = micros() - sendTime;
  
  // Update rolling latency buffer
  _latencyBuffer[_latencyBufferIndex] = currentLatency;
  _latencyBufferIndex = (_latencyBufferIndex + 1) % 8;
  
  // Calculate moving average
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += _latencyBuffer[i];
  }
  _averageLatency = sum / 8;
}

void WirelessSync::predictNextTick(uint32_t currentTick, uint64_t timestamp) {
  if (_lastReceivedTick == 0) {
    _lastReceivedTick = currentTick;
    return;
  }
  
  // Calculate tick interval and adjust for drift
  uint32_t tickInterval = uClock.bpmToMicroSeconds(uClock.getTempo()) / 24;
  _predictedNextTick = currentTick + 1;
  
  // Calculate drift based on received timestamp vs predicted
  uint64_t expectedTime = timestamp + tickInterval;
  uint64_t actualTime = micros();
  int32_t drift = actualTime - expectedTime;
  
  // Adjust drift correction factor (small adjustments)
  if (abs(drift) > 100) { // More than 100μs drift
    _driftCorrection += (drift > 0 ? 0.0001f : -0.0001f);
    _driftCorrection = constrain(_driftCorrection, 0.9f, 1.1f);
  }
  
  _lastReceivedTick = currentTick;
} 