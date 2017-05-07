-- trivial protocol example
-- declare our protocol
-- wireshark  -X lua_script:/mpudp.lua
mpudp_protocol = Proto("MPUDP","MPUDP Protocol")
function mpudp_protocol.init()
    pkt_cnt = 0
end

-- value-string maps

-- Create the protocol fields
local f = mpudp_protocol.fields
-- local format = { "Text", "Binary", [10] = "Special"}

f.version = ProtoField.uint8 ("MPUDP.version", "Version")
f.ohlen = ProtoField.uint8 ("MPUDP.ohlen", "Optional Header Length")
f.ifcSrc = ProtoField.uint8("MPUDP.ifcsrc", "Source Interface Id")
f.ifcDst = ProtoField.uint8("MPUDP.ifcdst", "Destination Interface Id")
f.flag = ProtoField.uint16 ("MPUDP.flag", "Flag")--, base.DEC, FLAG_VAL)

f.seqnum = ProtoField.uint16 ("MPUDP.seq", "Sequence Number")
f.acknum = ProtoField.uint16 ("MPUDP.ack", "Ack Number")

f.winsize = ProtoField.uint16 ("MPUDP.wins", "Window Size")
f.fprint = ProtoField.uint16 ("MPUDP.fprint", "Finger Print")

f.flowid = ProtoField.uint16 ("MPUDP.flowid", "Flow Id")
f.fseqno = ProtoField.uint16 ("MPUDP.fseqno", "Flow Sequence Number")

f.origackno = ProtoField.uint16 ("MPUDP.origackno", "Original Ack Num")

f.timeStamp = ProtoField.int64 ("MPUDP.timestamp", "Unix timestamp in micro second")

f.mydata = ProtoField.bytes  ("MPUDP.mydata", "Data")

f.flag_ack = ProtoField.bool ("MPUDP.flag_ack", "FLAG_ACK")
f.flag_dat = ProtoField.bool ("MPUDP.flag_dat", "FLAG_DAT")
f.flag_syn = ProtoField.bool ("MPUDP.flag_syn", "FLAG_SYN")
f.flag_ffn = ProtoField.bool ("MPUDP.flag_ffn", "FLAG_FFN")
f.flag_cfn = ProtoField.bool ("MPUDP.flag_cfn", "FLAG_CFN")
f.flag_fac = ProtoField.bool ("MPUDP.flag_fac", "FLAG_FAC")
f.flag_psh = ProtoField.bool ("MPUDP.flag_psh", "FLAG_PSH")
f.flag_csn = ProtoField.bool ("MPUDP.flag_csn", "FLAG_CSN")

--f.opthdr = ProtoField.none("MPUDP.opthdr", "OptionalHeader")
--f.flowid = ProtoField.uint16 ("OptionalHeader.flowid", "Flow Id")
f.flowack = ProtoField.uint16 ("MPUDP.flowack", "Flow Ack Number")

f.ifcType = ProtoField.uint8 ("MPUDP.ifc_type", "inteface type")
f.ifcId = ProtoField.uint8 ("MPUDP.ifc_type", "inteface type")
f.ifcIp = ProtoField.ipv4 ("MPUDP.ifc_ip", "interface Ip")

function bitwiseAnd(m, n)
    local c = 0
    local k = 1
    local a = m
    local b = n
    for x = 0, 15, 1
    do
        if a%2 == 1 and b%2 == 1
        then
            c = c + k
        end
        k = k*2
        a = a/2
        b = b/2
    end
    return c
end

function printLog(tree, flg)
    local flagIter = 1
    local val = 0
    local FLAG_VAL = { [0x01] = "ACK", [0x02] = "DAT", [0x04] = "SYN", [0x08] = "FFN", [0x10] = "CFN", [0x20]= "FAC", [0x40] = "PSH", [0x80] = "CSN"}
    local FLAG_TRE = { [0x01] = f.flag_ack, [0x02] = f.flag_dat, [0x04] = f.flag_syn, [0x08] = f.flag_ffn, [0x10] = f.flag_cfn, [0x20]= f.flag_fac, [0x40] = f.flag_psh, [0x80] = f.flag_csn}
    local k = 1
    local x = 0

    for i=0,15,1
    do
        x = flg%2
        if x > 0 then
            tree:append_text(", "..FLAG_VAL[k])
            tree:add(FLAG_TRE[k], x)
            x = 0
        end
        flg = math.floor(flg/2)
        k = k*2
    end
end

function showOptionalHeaders(buffer, offset, ohtype, tree)
    if ohtype == 1 then
        tree:append_text("ACK_HEADER")
        local flid = buffer(offset, 2)
        offset = offset + 2
        local ack = buffer(offset, 2)
        tree:add(f.flowid, flid)
        tree:add(f.flowack, ack)
    elseif ohtype == 2 then
        tree:append_text("IP_HEADER")
        local ifcid = buffer(offset, 1)
        offset = offset + 2
        local ip = buffer(offset, 4)
        tree:add(f.ifcId, ifcid)
        tree:add(f.ifcIp, ip)
    else 
        tree:append_text("UNKNOWN" .. ohtype)
    end
end

-- create a function to dissect it
function mpudp_protocol.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = "MPUDP"
    local subtree = tree:add(mpudp_protocol, buffer())
    local offset = 0

    local val = buffer(offset, 1)
    offset = offset + 1
    local ver = val:uint() / 0x10
    local ohlen = val:uint() % 0x10
    val = buffer(offset, 1)
    offset = offset + 1
    local ifcSrc = val:uint() / 0x10
    local ifcDst = val:uint() % 0x10
    local flg = buffer(offset, 2)
    offset = offset + 2

    local seqnum = buffer(offset, 2)
    offset = offset + 2
    local acknum = buffer(offset, 2)
    offset = offset + 2

    local wins = buffer(offset, 2)
    offset = offset + 2
    local fprint = buffer(offset, 2)
    offset = offset + 2

    local flowid = buffer(offset, 2)
    offset = offset + 2
    local fseqno = buffer(offset, 2)
    offset = offset + 2

    local origack = buffer(offset, 2)
    offset = offset + 2
    local padding = buffer(offset, 2)
    offset = offset + 2

    local timestamp = buffer(offset, 8)
    offset = offset + 8

    subtree:add(f.version, ver)
    subtree:add(f.ohlen, ohlen)
    subtree:add(f.ifcSrc, ifcSrc)
    subtree:add(f.ifcDst, ifcDst)
    flgTree = subtree:add(f.flag, flg)
    printLog(flgTree, flg:uint())

    
    subtree:add(f.seqnum, seqnum)
    subtree:add(f.acknum, acknum)

    subtree:add(f.winsize, wins)
    subtree:add(f.fprint, fprint)

    subtree:add(f.flowid, flowid)
    subtree:add(f.fseqno, fseqno)

    subtree:add(f.origackno, origack)

    subtree:add(f.timeStamp, timestamp)

    --local strAck = subtree:add(f.OptionalHeader)

    local iter = ohlen
    local len = 0
    local typ = 0
    while iter > 0 do
        typ = buffer(offset, 1)
        offset = offset + 1
        len = buffer(offset, 1)
        offset = offset + 1
        local stree = subtree:add("Optional Header: ")

        showOptionalHeaders (buffer, offset, typ:uint(), stree)
        offset = offset + len:uint() - 2

        iter = iter - 1
    end

    subtree:add(f.mydata, buffer(offset))
end
-- load the udp.port table
udp_table = DissectorTable.get("udp.port")
-- register our protocol to handle udp port 7777
udp_table:add(8989,mpudp_protocol)
