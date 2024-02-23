
export default class functions{
  private constructor() {
  }

  public static isReserveClickable(ReserveAccount: string, ReservePassword: string, ReserveLabel: string): boolean {
    return ReserveAccount !== '' && ReservePassword !== '' && ReserveLabel != '';
  }

  public static isDeleteClickable(DeleteAccount: string, DeleteLabel: string): boolean {
    return DeleteAccount !== '' && DeleteLabel != '';
  }

  public static isCheckClickable(CheckAccount: string, CheckLabel: string): boolean {
    return CheckAccount !== '' && CheckLabel != '';
  }

  public static isUpdateClickable(ReservePassword: string): boolean {
    return ReservePassword !== '';
  }

}


