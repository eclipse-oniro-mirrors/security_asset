import asset from '@ohos.security.asset';
import hilog from '@ohos.hilog';
import util from '@ohos.util';
import { BusinessError } from '@ohos.base';

export const TAG = "CommonAsset";

export const NORMAL_LABEL1 = "normal_label1"
export const NORMAL_LABEL2 = "normal_label2"
export const NORMAL_LABEL3 = "normal_label3"
export const NORMAL_LABEL4 = "normal_label4"
export const CRITICAL_LABEL1 = "critical_label1"
export const CRITICAL_LABEL2 = "critical_label2"
export const CRITICAL_LABEL3 = "critical_label3"
export const CRITICAL_LABEL4 = "critical_label4"

export function stringToArray(str: string): Uint8Array {
  let textEncoder = new util.TextEncoder();
  return textEncoder.encodeInto(str);
}

export function arrayToString(arr: Uint8Array): string {
  let textDecoder = util.TextDecoder.create("utf-8", { ignoreBOM: true });
  let str = textDecoder.decodeWithStream(arr, { stream: false })
  return str;
}

export function sleep(ms: number) {
  return new Promise<void>(resolve => setTimeout(resolve, ms));
}

export async function addAssetPromise(str: string): Promise<boolean> {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
  try {
    await asset.add(attr)
    hilog.info(0x0000, TAG, 'AddAssetPromise Success');
    return true
  } catch (error) {
    hilog.info(0x0000, TAG, 'AddAssetPromise Failed, code: ' + error.code + ', msg: ' + error.message);
    return false
  }
}

export function addAsset(str: string): boolean {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
  try {
    asset.addSync(attr);
    hilog.info(0x0000, TAG, 'AddAssetPromise Success');
    return true
  } catch (error) {
    hilog.info(0x0000, TAG, 'AddAssetPromise Failed, code: ' + error.code + ', msg: ' + error.message);
    return false
  }
}

export async function addAssetPromiseResult(map: asset.AssetMap): Promise<number> {
  try {
    await asset.add(map)
    return 0
  } catch (error) {
    return error.code
  }
}

export function addAssetResult(map: asset.AssetMap): number {
  try {
    asset.addSync(map)
    return 0
  } catch (error) {
    return error.code
  }
}

export function addAssetParamNumError(): number {
  try {
    asset.addSync()
    return 0
  } catch (error) {
    return error.code
  }
}

export async function queryContentPromise(str: string): Promise<asset.AssetMap> {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  attr.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
  try {
    let data = await asset.query(attr)
    return data[0]
  } catch (error) {
    return new Map()
  }
}

export function queryContent(str: string): asset.AssetMap {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  attr.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
  try {
    let data = asset.querySync(attr)
    return data[0]
  } catch (error) {
    return new Map()
  }
}

export function queryCount(): number {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ATTRIBUTES)
  try {
    let data = asset.querySync(attr)
    return data.length
  } catch (error) {
    return 0
  }
}

export function queryError(attr: asset.AssetMap): number {
  try {
    asset.querySync(attr)
    return 0
  } catch (error) {
    return error.code
  }
}

export function queryAssetParamNumError(): number {
  try {
    asset.querySync()
    return 0
  } catch (error) {
    return error.code
  }
}

export async function queryAttrPromise(str: string): Promise<asset.AssetMap> {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  attr.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ATTRIBUTES)
  try {
    let data = await asset.query(attr)
    return data[0]
  } catch (error) {
    return new Map()
  }
}

export function queryAttr(str: string): asset.AssetMap {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  attr.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ATTRIBUTES)
  try {
    let data = asset.querySync(attr)
    return data[0]
  } catch (error) {
    return new Map()
  }
}

export async function queryAssetByAlias(str: string): Promise<number> {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  attr.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ATTRIBUTES)
  try {
    await asset.query(attr)
    return 0
  } catch (error) {
    return error.code
  }
}

export async function queryAssetPromiseResult(map: asset.AssetMap): Promise<number> {
  try {
    await asset.query(map)
    return 0
  } catch (error) {
    return error.code
  }
}

export async function removeAssetPromise(str: string): Promise<boolean> {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  try {
    await asset.remove(attr)
    return true
  } catch (error) {
    return false
  }
}

export function removeAsset(str: string): boolean {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  try {
    asset.removeSync(attr)
    return true
  } catch (error) {
    return false
  }
}

export function removeAssetError(attr: asset.AssetMap): number {
  try {
    asset.removeSync(attr)
    return 0
  } catch (error) {
    return error.code
  }
}

export function removeAssetParamNumError(): number {
  try {
    asset.removeSync()
    return 0
  } catch (error) {
    return error.code
  }
}

export async function removeAll(): Promise<boolean> {
  let attr: asset.AssetMap = new Map();
  try {
    await asset.remove(attr)
    return true
  } catch (error) {
    return false
  }
}

export async function removeAssetPromiseResult(map: asset.AssetMap): Promise<number> {
  try {
    await asset.remove(map)
    return 0
  } catch (error) {
    return error.code
  }
}

export async function updateAssetByAlias(str: string, updateMap: asset.AssetMap): Promise<number> {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  try {
    await asset.update(attr, updateMap)
    return 0
  } catch (error) {
    return error.code
  }
}

export function updateAssetByAliasSync(str: string, updateMap: asset.AssetMap): number {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  try {
    asset.updateSync(attr, updateMap)
    return 0
  } catch (error) {
    return error.code
  }
}

export function updateAssetSync(attr: asset.AssetMap, updateMap: asset.AssetMap): number {

  try {
    asset.updateSync(attr, updateMap)
    return 0
  } catch (error) {
    return error.code
  }
}

export function updateAssetParamNumError(str: string): number {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.ALIAS, stringToArray(str))
  try {
    asset.updateSync(attr)
    return 0
  } catch (error) {
    return error.code
  }
}

export async function updateQueriedAsset(query: asset.AssetMap, str: string): Promise<number> {
  let update: asset.AssetMap = new Map()
  update.set(asset.Tag.SECRET, stringToArray(str))
  try {
    await asset.update(query, update)
    return 0
  } catch (error) {
    return error.code
  }
}

export async function preQueryAssetPromise(map: asset.AssetMap): Promise<Uint8Array> {
  try {
    return await asset.preQuery(map)
  } catch (error) {
    return new Uint8Array(0)
  }
}

export function preQueryAsset(map: asset.AssetMap): Uint8Array {
  try {
    return asset.preQuerySync(map)
  } catch (error) {
    return new Uint8Array(0)
  }
}

export async function preQueryAssetPromiseResult(map: asset.AssetMap): Promise<number> {
  try {
    await asset.preQuery(map)
    return 0
  } catch (error) {
    return error.code
  }
}

export function preQueryAssetResult(map: asset.AssetMap): number {
  try {
    asset.preQuerySync(map)
    return 0
  } catch (error) {
    return error.code
  }
}

export function preQueryAssetParamNumError(): number {
  try {
    asset.preQuerySync()
    return 0
  } catch (error) {
    return error.code
  }
}

export async function postQueryAssetPromise(challenge: Uint8Array): Promise<boolean> {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.AUTH_CHALLENGE, challenge);
  try {
    await asset.postQuery(attr);
    return true
  } catch (error) {
    return false
  }
}

export function postQueryAsset(challenge: Uint8Array): boolean {
  let attr: asset.AssetMap = new Map();
  attr.set(asset.Tag.AUTH_CHALLENGE, challenge);
  try {
    asset.postQuerySync(attr);
    return true
  } catch (error) {
    return false
  }
}

export async function postQueryAssetPromiseResult(map: asset.AssetMap): Promise<number> {
  try {
    await asset.postQuery(map)
    return 0
  } catch (error) {
    return error.code
  }
}

export function postQueryAssetParamNumError(): number {
  try {
    asset.postQuery()
    return 0
  } catch (error) {
    return error.code
  }
}

export function postQueryAssetResult(map: asset.AssetMap): number {
  try {
    asset.postQuery(map)
    return 0
  } catch (error) {
    return error.code
  }
}

export async function addAllTagsAssetPromise(alias: string): Promise<number> {
  try {
    let attr: asset.AssetMap = new Map();
    attr.set(asset.Tag.ALIAS, stringToArray(alias))
    attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
    attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray(NORMAL_LABEL1))
    attr.set(asset.Tag.DATA_LABEL_NORMAL_2, stringToArray(NORMAL_LABEL2))
    attr.set(asset.Tag.DATA_LABEL_NORMAL_3, stringToArray(NORMAL_LABEL3))
    attr.set(asset.Tag.DATA_LABEL_NORMAL_4, stringToArray(NORMAL_LABEL4))
    attr.set(asset.Tag.DATA_LABEL_CRITICAL_1, stringToArray(CRITICAL_LABEL1))
    attr.set(asset.Tag.DATA_LABEL_CRITICAL_2, stringToArray(CRITICAL_LABEL2))
    attr.set(asset.Tag.DATA_LABEL_CRITICAL_3, stringToArray(CRITICAL_LABEL3))
    attr.set(asset.Tag.DATA_LABEL_CRITICAL_4, stringToArray(CRITICAL_LABEL4))
    attr.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON)
    attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
    attr.set(asset.Tag.SYNC_TYPE, asset.SyncType.THIS_DEVICE)
    attr.set(asset.Tag.REQUIRE_PASSWORD_SET, false)
    await asset.add(attr)
    return 0
  } catch (error) {
    return error.code
  }
}
