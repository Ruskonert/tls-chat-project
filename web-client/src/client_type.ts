/**
 * PackingMessage 클래스를 정의합니다.
 */
export class PackingMessage
{
  len : number;
  
  command: number;
  
  data: string;

  
  static ConvertMessage(command : number, data: string) : PackingMessage
  {
    var newMessage = new PackingMessage(data.length, command, data);
    return newMessage;
  }


  static ConvertMessageFromBinary(data : string)
  {
    const magicNumber = data.substring(1, 4);
    if(!(Buffer.from(magicNumber).equals(Buffer.from([239, 191, 119, 102])))) {
      console.log("Invalid magic number of packet data");
      return null;
    }
    
    const commandNum = parseInt(("0x" + data.substring(5, 8)).replace(/^#/, ''), 16);
    const messageLen = parseInt(("0x" + data.substring(7, 11)).replace(/^#/, ''), 16);

    var packingMessage = new PackingMessage(messageLen, commandNum, data.substring(12));
    return packingMessage;
  }


  constructor(len: number, command_type: number, data: string)
  {
    this.len = len;
    this.command = command_type;
    this.data = data;
  }

  public toString() : string
  {
    var dataLengthStr = this.data.length.toString(16).padStart(4, '0');
    var commandStr = this.command.toString(16).padStart(4, '0');
    var magicNumber = '';

    magicNumber += String.fromCharCode(0x99);
    magicNumber += String.fromCharCode(0x88);
    magicNumber += String.fromCharCode(0x77);
    magicNumber += String.fromCharCode(0x66);

    var completeStr = `${magicNumber}${commandStr}${dataLengthStr}${this.data}`;
    return completeStr;
  }
}

export function isPackingMessage(buf : any) {
  return Buffer.from(buf.substring(1, 4)).equals(Buffer.from([239, 191, 119, 102]));
}