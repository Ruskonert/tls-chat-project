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

  static ConvertMessageFromBinary(data : Buffer)
  {
    
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