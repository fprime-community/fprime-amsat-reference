module Components {
    @ AMSat USB Sound Component - Audio Capture and Transmission
    active component USBSoundCard {
        
        # ===============================================
        # BASIC COMMANDS - Audio capture and transmission
        # ===============================================
        
        @ Start audio capture
        async command START_CAPTURE opcode 0
        
        @ Stop audio capture  
        async command STOP_CAPTURE opcode 1
        
        @ Start audio transmission
        async command START_TRANSMISSION opcode 2
        
        @ Stop audio transmission
        async command STOP_TRANSMISSION opcode 3
        
        @ Send test packet
        async command SEND_TEST_PACKET opcode 4
        
        # ===============================================
        # BASIC TELEMETRY - Monitor audio health and transmission
        # ===============================================
        
        @ Current audio input level (0-255)
        telemetry AUDIO_INPUT_LEVEL: U32 id 0x1001
        
        @ Audio device connection status
        telemetry DEVICE_CONNECTED: bool id 0x1002
        
        @ Number of audio frames processed
        telemetry FRAMES_PROCESSED: U32 id 0x1003
        
        @ Peak audio level in last second
        telemetry AUDIO_PEAK_LEVEL: U32 id 0x1004
        
        @ Transmission status
        telemetry TRANSMISSION_ACTIVE: bool id 0x1005
        
        @ Number of packets transmitted
        telemetry PACKETS_TRANSMITTED: U32 id 0x1006
        
        @ Last transmission timestamp (seconds)
        telemetry LAST_TRANSMISSION_TIME: U32 id 0x1007
        
        # ===============================================
        # APRS TELEMETRY - CubeSat telemetry from APRS packets
        # ===============================================
        
        @ APRS Latitude in decimal degrees
        telemetry APRS_LATITUDE: F32 id 0x01
        
        @ APRS Longitude in decimal degrees
        telemetry APRS_LONGITUDE: F32 id 0x02
        
        @ APRS Battery voltage in volts
        telemetry APRS_BATTERY: F32 id 0x03
        
        @ APRS Temperature in Celsius
        telemetry APRS_TEMPERATURE: F32 id 0x04
        
        @ APRS packet count received
        telemetry APRS_PACKET_COUNT: U32 id 0x05
        
        @ APRS signal strength (if available)
        telemetry APRS_SIGNAL_STRENGTH: F32 id 0x06
        
        # ===============================================
        # BASIC EVENTS - Essential logging
        # ===============================================
        
        @ Audio capture started
        event AUDIO_CAPTURE_STARTED severity activity low id 0 format "Audio capture started"
        
        @ Audio capture stopped
        event AUDIO_CAPTURE_STOPPED severity activity low id 1 format "Audio capture stopped"
        
        @ USB sound card disconnected
        event DEVICE_DISCONNECTED severity warning high id 2 format "USB sound card disconnected"
        
        @ Audio capture already started
        event AUDIO_CAPTURE_ALREADY_STARTED severity warning high id 3 format "Audio capture already started"
        
        @ Audio level too high
        event AUDIO_LEVEL_HIGH severity warning low id 4 format "Audio input level too high"
        
        @ Audio transmission started
        event AUDIO_TRANSMISSION_STARTED severity activity low id 5 format "Audio transmission started"
        
        @ Audio transmission stopped
        event AUDIO_TRANSMISSION_STOPPED severity activity low id 6 format "Audio transmission stopped"
        
        @ Audio transmission already started
        event AUDIO_TRANSMISSION_ALREADY_STARTED severity warning high id 7 format "Audio transmission already started"
        
        @ Packet transmitted successfully
        event PACKET_TRANSMITTED severity activity low id 8 format "Packet transmitted successfully"
        
        @ Transmission error
        event TRANSMISSION_ERROR severity warning high id 9 format "Transmission error occurred"
        
        # ===============================================
        # APRS EVENTS - CubeSat telemetry events
        # ===============================================
        
        @ APRS packet received from CubeSat
        event APRS_PACKET_RECEIVED(callsign: string size 16) \
            severity activity low id 0x10 \
            format "Received APRS packet from {}"
        
        @ APRS parsing error
        event APRS_PARSE_ERROR(error: string size 64) \
            severity warning high id 0x11 \
            format "APRS parse error: {}"
        
        @ APRS position update
        event APRS_POSITION_UPDATE(lat: F32, lon: F32) \
            severity activity low id 0x12 \
            format "APRS position update: LAT={.6f}, LON={.6f}"
        
        @ APRS telemetry update
        event APRS_TELEMETRY_UPDATE(battery: F32, temp: F32) \
            severity activity low id 0x13 \
            format "APRS telemetry: BAT={.1f}V, TEMP={.1f}C"
        
        # ===============================================
        # SCHEDULED INPUT - For periodic audio processing
        # ===============================================
        
        @ Port for receiving periodic calls from rate group
        sync input port run: Svc.Sched
        
        # ===============================================
        # OUTPUT PORTS - For sending FPrime packets
        # ===============================================
        
        @ Port for sending FPrime packets
        output port packetOut: Fw.Com
        
        @ Port for sending buffer packets
        output port bufferOut: Fw.BufferSend
        
        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller
        @ Port for sending command registrations
        command reg port cmdRegOut
        @ Port for receiving commands
        command recv port cmdIn
        @ Port for sending command responses
        command resp port cmdResponseOut
        @ Port for sending textual representation of events
        text event port logTextOut
        @ Port for sending events to downlink
        event port logOut
        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut
        @ Port to return the value of a parameter
        param get port prmGetOut
        @Port to set the value of a parameter
        param set port prmSetOut
    }
}