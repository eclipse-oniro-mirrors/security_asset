import asset from '@ohos.security.asset';
import { sleep, stringToArray } from './common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';
import { BusinessError } from '@ohos.base';

let promiseCaseNum = 100
let promise_base_alias = '11111111'
let aliasNum = 10
let secret1 = '1111111111111111'
let secret2 = '2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222'
function getRandomNum(numRange: number): number {
  return Math.floor(Math.random() * numRange)
}

export default function performanceAssetPromiseTest() {
  describe('interfaceConcurrencyPromiseTest', () => {
    beforeAll(async () => {
      for (let index = 0; index < aliasNum; index++) {
        let alias = promise_base_alias.concat(index.toString())
        let attr: asset.AssetMap = new Map();
        attr.set(asset.Tag.ALIAS, stringToArray(alias))
        attr.set(asset.Tag.SECRET, stringToArray(secret1))
        attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label1"))
        attr.set(asset.Tag.DATA_LABEL_NORMAL_2, stringToArray("normal_label2"))
        attr.set(asset.Tag.DATA_LABEL_NORMAL_3, stringToArray("normal_label3"))
        attr.set(asset.Tag.DATA_LABEL_NORMAL_4, stringToArray("normal_label4"))
        attr.set(asset.Tag.DATA_LABEL_CRITICAL_1, stringToArray("critical_label1"))
        attr.set(asset.Tag.DATA_LABEL_CRITICAL_2, stringToArray("critical_label2"))
        attr.set(asset.Tag.DATA_LABEL_CRITICAL_3, stringToArray("critical_label3"))
        attr.set(asset.Tag.DATA_LABEL_CRITICAL_4, stringToArray("critical_label4"))
        attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE)
        attr.set(asset.Tag.IS_PERSISTENT, true)
        await asset.add(attr)
      }
    })
    beforeEach(() => {
    })
    afterEach(async () => {
      await sleep((promiseCaseNum) * 100 )
    })
    afterAll(() => {
    })

    it('queryDataPromise', 0, async () => {
      try {
        for (let index = 0; index < promiseCaseNum; index++) {
          let alias = promise_base_alias.concat(getRandomNum(aliasNum).toString())
          let attr: asset.AssetMap = new Map();
          attr.set(asset.Tag.ALIAS, stringToArray(alias))
          attr.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
          asset.query(attr).then(() => {
            expect(true).assertTrue()
          }).catch((error: BusinessError) => {
              expect([asset.ErrorCode.DUPLICATED, asset.ErrorCode.NOT_FOUND]).assertContain(error.code)
          })
        }
      } catch (error) {
        expect().assertFail()
      }
    })

    it('queryBatchDataPromise', 0, async () => {
      try {
        for (let index = 0; index < promiseCaseNum; index++) {
          let attr: asset.AssetMap = new Map();
          asset.query(attr).then(() => {
            expect(true).assertTrue()
          }).catch((error: BusinessError) => {
            expect([asset.ErrorCode.DUPLICATED, asset.ErrorCode.NOT_FOUND]).assertContain(error.code)
          })
        }
      } catch (error) {
        expect().assertFail()
      }
    })

    it('updateDataPromise', 0, async () => {
      let attr2: asset.AssetMap = new Map();
      attr2.set(asset.Tag.SECRET, stringToArray(secret2))
      let alias_list = new Array<String>();
      try {
        for (let index = 0; index < promiseCaseNum; index++) {
          let alias = promise_base_alias.concat(getRandomNum(aliasNum).toString())
          alias_list.push(alias)
          let attr: asset.AssetMap = new Map();
          attr.set(asset.Tag.ALIAS, stringToArray(alias))
          asset.update(attr, attr2).then(() => {
            expect(true).assertTrue()
          }).catch((error: BusinessError) => {
            expect([asset.ErrorCode.DUPLICATED, asset.ErrorCode.NOT_FOUND]).assertContain(error.code)
          })
        }
      } catch (error) {
        expect().assertFail()
      }
    })

    it('deleteDataPromise', 0, async () => {
      try {
        for (let index = 0; index < promiseCaseNum; index++) {
          let alias = promise_base_alias.concat(getRandomNum(aliasNum).toString())
          let attr: asset.AssetMap = new Map();
          attr.set(asset.Tag.ALIAS, stringToArray(alias))
          asset.remove(attr).then(() => {
            expect(true).assertTrue()
          }).catch((error: BusinessError) => {
            expect([asset.ErrorCode.DUPLICATED, asset.ErrorCode.NOT_FOUND]).assertContain(error.code)
          })
        }
      } catch (error) {
        expect().assertFail()
      }
    })
  })
}